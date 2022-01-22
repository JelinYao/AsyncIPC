#pragma once
#include "async_ipc.h"
#include <thread>
#include <memory>
#include <list>

#include "named_pipe_define.h"

class IPipeDelegate;
class NamedPipeImpl
    : public INamedPipe
{
public:
    NamedPipeImpl();
    virtual ~NamedPipeImpl();

    virtual bool Create(const wchar_t* pipe_name, PipeType type, IPipeDelegate* delegate);
    virtual void Exit();
    virtual void Send(const char* msg);

protected:
    bool CreatePipeServer(const wchar_t* pipe_name);
    bool CreatePipeClient(const wchar_t* pipe_name);
    void ClosePipe();

    virtual bool PipeWrite(void* data, uint32_t size) = 0;
    virtual bool PipeRead() = 0;

    static void ThreadProc(void* param);
    virtual void DoWork() = 0;

protected:
    bool need_exit_;
    HANDLE pipe_handle_;
    IPipeDelegate* delegate_;
    std::unique_ptr<std::thread> async_thread_;
    std::list<std::string> send_list_;
    std::mutex send_mutex_;
    HANDLE wait_event_;
};

