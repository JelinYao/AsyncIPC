#include "stdafx.h"
#include "named_pipe_overlapped.h"

NamedPipeOverlapped::NamedPipeOverlapped()
    : NamedPipeImpl()
    , send_flag_(0)
    , recv_flag_(0)
{
    ZeroMemory(&overlappeds_, sizeof(overlappeds_));
}

NamedPipeOverlapped::~NamedPipeOverlapped()
{
}

bool NamedPipeOverlapped::Create(LPCWSTR pipe_name, PipeType type, IPipeDelegate* delegate)
{
    overlappeds_[0].type = OverlappedType::OverlappedWrite;
    overlappeds_[0].ol.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    overlappeds_[1].type = OverlappedType::OverlappedRead;
    overlappeds_[1].ol.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    return NamedPipeImpl::Create(pipe_name, type, delegate);
}

void NamedPipeOverlapped::Exit()
{
    NamedPipeImpl::Exit();
    if (overlappeds_[0].ol.hEvent) {
        ::CloseHandle(overlappeds_[0].ol.hEvent);
        overlappeds_[0].ol.hEvent = NULL;
    }
    if (overlappeds_[1].ol.hEvent) {
        ::CloseHandle(overlappeds_[1].ol.hEvent);
        overlappeds_[1].ol.hEvent = NULL;
    }
}

bool NamedPipeOverlapped::PipeWrite(void* data, uint32_t size)
{
    BOOL result = ::WriteFile(pipe_handle_, data, size, &overlappeds_[0].trans_bytes, (LPOVERLAPPED)&overlappeds_[0]);
    DWORD error_code = ::GetLastError();
    if (!result && error_code != ERROR_IO_PENDING) {
        LOG(ERROR) << "WriteFile failed, error code: " << error_code;
        return false;
    }
    send_flag_++;
    return true;
}

bool NamedPipeOverlapped::PipeRead()
{
    BOOL result = ::ReadFile(pipe_handle_, overlappeds_[1].buffer, kPipeBufferSize, &overlappeds_[1].trans_bytes, (LPOVERLAPPED)&overlappeds_[1]);
    DWORD error_code = ::GetLastError();
    if (!result && error_code != ERROR_IO_PENDING) {
        LOG(ERROR) << "ReadFile failed, error code: " << error_code;
        return false;
    }
    recv_flag_++;
    return true;
}

void NamedPipeOverlapped::DoWork()
{
    HANDLE handles[3] = { wait_event_, overlappeds_[0].ol.hEvent, overlappeds_[1].ol.hEvent };
    DWORD wait_result = 0;
    PipeRead();
    while (!need_exit_) {
        if (send_flag_ < 1) {
            std::unique_lock<std::mutex> auto_lock(send_mutex_);
            if (!send_list_.empty()) {
                auto msg = std::move(send_list_.front());
                send_list_.pop_front();
                PipeWrite((void*)msg.c_str(), msg.size());
            }
        }
        wait_result = ::WaitForMultipleObjects(3, handles, FALSE, INFINITE);
        if (need_exit_) {
            break;
        }
        switch (wait_result)
        {
        case WAIT_OBJECT_0: {
            break;
        }
        case WAIT_OBJECT_0 + 1: {
            OnWriteComplete();
            break;
        }
        case WAIT_OBJECT_0 + 2: {
            OnReadComplete();
            break;
        }
        default: {
            LOG(ERROR) << "WaitForMultipleObjects failed, error code: " << ::GetLastError();
            break;
        }
        }
    }
}

bool NamedPipeOverlapped::OnWriteComplete()
{
    send_flag_--;
    int recv_count = overlappeds_[0].ol.InternalHigh;
    if (delegate_) {
        delegate_->OnSend(recv_count);
    }
    return true;
}

bool NamedPipeOverlapped::OnReadComplete()
{
    recv_flag_--;
    int recv_count = overlappeds_[1].ol.InternalHigh;
    overlappeds_[1].buffer[recv_count] = '\0';
    DWORD bytes_trans;
    BOOL ol_result = ::GetOverlappedResult(pipe_handle_, &overlappeds_[1].ol, &bytes_trans, FALSE);
    if (!ol_result) {
        DWORD code = ::GetLastError();
        if (code != ERROR_MORE_DATA) {
            LOG(ERROR) << "GetOverlappedResult failed, last error: " << code;
            return false;
        }
        LOG(INFO) << "read buffer is not enough";
        recv_buffer_.append(overlappeds_[1].buffer);
        PipeRead();
        return true;
    }
    bool is_long_data = !recv_buffer_.empty();
    if (is_long_data) {
        recv_buffer_.append(overlappeds_[1].buffer);
    }
    if (delegate_) {
        if (is_long_data) {
            delegate_->OnRecv((void*)recv_buffer_.c_str(), recv_buffer_.size());
            recv_buffer_.clear();
        }
        else {
            delegate_->OnRecv(overlappeds_[1].buffer, recv_count);
        }
    }
    PipeRead();
    return true;
}
