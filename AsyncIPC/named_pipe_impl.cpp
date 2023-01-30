#include "stdafx.h"
#include "named_pipe_impl.h"
#include "named_pipe_define.h"

NamedPipeImpl::NamedPipeImpl()
    : need_exit_(false)
    , pipe_handle_(INVALID_HANDLE_VALUE)
    , delegate_(nullptr)
    , wait_event_(NULL)
{
}

NamedPipeImpl::~NamedPipeImpl()
{
    Exit();
}

bool NamedPipeImpl::Create(const wchar_t* pipe_name, PipeType type, IPipeDelegate* delegate)
{
    if (pipe_name == nullptr) {
        LOG(ERROR) << "Create pipe failed, pipe_name is null";
        return false;
    }
    BOOL result = FALSE;
    delegate_ = delegate;
    switch (type)
    {
    case PIPE_SERVER:
        result = CreatePipeServer(pipe_name);
        break;
    case PIPE_CLIENT:
        result = CreatePipeClient(pipe_name);
        break;
    default:
        break;
    }
    if (!result) {
        LOG(ERROR) << "CreatePipe failed, PipeType: " << type;
        return FALSE;
    }
    wait_event_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    async_thread_ = std::make_unique<std::thread>(ThreadProc, this);
    return TRUE;
}

void NamedPipeImpl::Exit()
{
    need_exit_ = true;
    if (wait_event_ == NULL) {
        return;
    }
    ::SetEvent(wait_event_);
    if (async_thread_ != nullptr && async_thread_->joinable()) {
        async_thread_->join();
    }
    ClosePipe();
    ::CloseHandle(wait_event_);
    wait_event_ = NULL;
}

void NamedPipeImpl::Send(const char* msg)
{
    if (msg == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    send_list_.push_back(msg);
    ::SetEvent(wait_event_);
}

bool NamedPipeImpl::CreatePipeServer(const wchar_t* pipe_name)
{
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        ClosePipe();
    }
    pipe_handle_ = ::CreateNamedPipe(pipe_name,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        kPipeBufferSize,
        kPipeBufferSize,
        kPipeTimeout,
        NULL);
    if (delegate_) {
        delegate_->OnCreate(pipe_handle_ != INVALID_HANDLE_VALUE);
    }
    if (pipe_handle_ == INVALID_HANDLE_VALUE) {
        LOG(ERROR) << "CreateNamedPipe error, code: " << GetLastError();
        return FALSE;
    }
    OVERLAPPED ol = { 0 };
    ol.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    ::ConnectNamedPipe(pipe_handle_, &ol);
    // 等待10秒，超时后返回失败，避免无限等待连接造成程序卡死。
    bool connected = ::WaitForSingleObject(ol.hEvent, 10 * 1000) != WAIT_TIMEOUT;
    ::CloseHandle(ol.hEvent);
    if (!connected) {
        LOG(ERROR) << L"ConnectNamedPipe error, code: %u" << GetLastError();
    }
    if (delegate_) {
        delegate_->OnConnected(connected);
    }
    return true;
}

bool NamedPipeImpl::CreatePipeClient(const wchar_t* pipe_name)
{
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        ClosePipe();
    }
    while (true) {
        pipe_handle_ = ::CreateFile(pipe_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL);
        if (pipe_handle_ != INVALID_HANDLE_VALUE) {
            break;
        }
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            LOG(ERROR) << "Could not open pipe, error code: " << GetLastError();
            return FALSE;
        }
        if (!::WaitNamedPipe(pipe_name, 0)) {
            LOG(ERROR) << "WaitNamedPipe Failed, error code: " << GetLastError();
            if (delegate_) {
                delegate_->OnConnected(false);
            }
            return false;
        }
    }
    if (delegate_) {
        delegate_->OnConnected(true);
    }
    LOG(INFO) << "Connect pipe server succeed";
    return true;
}

void NamedPipeImpl::ClosePipe()
{
    LOG(INFO) << "ClosePipe begin";
    if (pipe_handle_ != INVALID_HANDLE_VALUE) {
        ::DisconnectNamedPipe(pipe_handle_);
        ::CloseHandle(pipe_handle_);
        pipe_handle_ = INVALID_HANDLE_VALUE;
    }
}

void NamedPipeImpl::ThreadProc(void* param)
{
    NamedPipeImpl* pipe = (NamedPipeImpl*)param;
    pipe->DoWork();
}
