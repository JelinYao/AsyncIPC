#include "stdafx.h"
#include "named_pipe_async.h"

NamedPipeAsync::NamedPipeAsync()
    : NamedPipeImpl()
{
}

NamedPipeAsync::~NamedPipeAsync()
{
}

bool NamedPipeAsync::PipeWrite(void* data, uint32_t size)
{
    int ol_size = sizeof(PipeOverlapped);
    PipeOverlapped* ol_data = (PipeOverlapped*)malloc(ol_size);
    ZeroMemory(ol_data, ol_size);
    ol_data->param = this;
    BOOL result = ::WriteFileEx(pipe_handle_, data, size, (LPOVERLAPPED)ol_data, OnWriteCompletionRoutine);
    if (!result && GetLastError() != ERROR_IO_PENDING) {
        LOG(ERROR) << "WriteFileEx failed, error code: " << GetLastError();
        return false;
    }
    return true;
}

bool NamedPipeAsync::PipeRead()
{
    int ol_size = sizeof(PipeOverlapped);
    PipeOverlapped* ol_data = (PipeOverlapped*)malloc(ol_size);
    ZeroMemory(ol_data, ol_size);
    ol_data->param = this;
    BOOL result = ::ReadFileEx(pipe_handle_, ol_data->buffer, kPipeBufferSize, (LPOVERLAPPED)ol_data, OnReadCompletionRoutine);
    if (!result && GetLastError() != ERROR_IO_PENDING) {
        LOG(ERROR) << "ReadFileEx failed, error code: " << GetLastError();
        return false;
    }
    return true;
}

void WINAPI NamedPipeAsync::OnWriteCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    PipeOverlapped* ol_data = (PipeOverlapped*)lpOverlapped;
    if (dwErrorCode != 0) {
        LOG(ERROR) << "OnWriteCompletionRoutine failed, error code: " << dwErrorCode;
        free(ol_data);
        return;
    }
    NamedPipeAsync* server = (NamedPipeAsync*)ol_data->param;
    if (server->delegate_) {
        server->delegate_->OnSend(dwNumberOfBytesTransfered);
    }
    free(ol_data);
}

void WINAPI NamedPipeAsync::OnReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    PipeOverlapped* ol_data = (PipeOverlapped*)lpOverlapped;
    NamedPipeAsync* server = (NamedPipeAsync*)ol_data->param;
    if (dwErrorCode != 0) {
        if ((dwErrorCode == ERROR_PIPE_NOT_CONNECTED || dwErrorCode == ERROR_BROKEN_PIPE) && server->delegate_) {
            server->delegate_->OnDisconnected();
        }
        LOG(ERROR) << "OnWriteCompletionRoutine failed, error code: " << dwErrorCode;
        free(ol_data);
        return;
    }
    ol_data->buffer[dwNumberOfBytesTransfered] = '\0';
    server->PipeRead();
    DWORD bytes_trans;
    BOOL ol_result = ::GetOverlappedResult(server->pipe_handle_, lpOverlapped, &bytes_trans, FALSE);
    if (!ol_result && GetLastError() == ERROR_MORE_DATA) {
        LOG(INFO) << "read buffer is not enough";
        server->recv_buffer_.append(ol_data->buffer);
        free(ol_data);
        return;
    }
    bool is_long_data = !server->recv_buffer_.empty();
    if (is_long_data) {
        server->recv_buffer_.append(ol_data->buffer);
    }
    LOG(INFO) << "OnReadCompletionRoutine dwNumberOfBytesTransfered: " << dwNumberOfBytesTransfered << ", data: " << ol_data->buffer;
    if (server->delegate_) {
        if (is_long_data) {
            server->delegate_->OnRecv((void*)server->recv_buffer_.c_str(), server->recv_buffer_.size());
            server->recv_buffer_.clear();
        }
        else {
            server->delegate_->OnRecv(ol_data->buffer, dwNumberOfBytesTransfered);
        }
    }
    free(ol_data);
}

void NamedPipeAsync::DoWork()
{
    PipeRead();
    while (!need_exit_) {
        DWORD wait_result = ::WaitForSingleObjectEx(wait_event_, INFINITE, TRUE);
        if (need_exit_)
            break;
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_IO_COMPLETION)
            break;
        std::unique_lock<std::mutex> auto_lock(send_mutex_);
        if (send_list_.empty())
            continue;
        auto msg = std::move(send_list_.front());
        send_list_.pop_front();
        auto_lock.unlock();
        if (!PipeWrite((void*)msg.c_str(), msg.size())) {
            LOG(ERROR) << "Pipe disconnected";
            if (delegate_) {
                delegate_->OnDisconnected();
            }
            break;
        }
    }
}
