#pragma once

#ifdef ASYNCIPC_EXPORTS
#define ASYNCIPC_API extern "C" __declspec(dllexport)
#else
#define ASYNCIPC_API extern "C" __declspec(dllimport)
#endif

enum PipeType {
    PIPE_SERVER = 0,
    PIPE_CLIENT,
};

enum PipeImplType {
    PIPE_ASYNC = 0,
    PIPE_OVERLAPPED,
};

class IPipeDelegate {
public:
    virtual void OnCreate(bool) = 0;
    virtual void OnConnected(bool) = 0;
    virtual void OnSend(int size) = 0;
    virtual void OnRecv(void* data, int size) = 0;
    virtual void OnDisconnected() = 0;
};

class INamedPipe
{
public:
    virtual bool Create(const wchar_t* pipe_name, PipeType type, IPipeDelegate* delegate) = 0;
    virtual void Exit() = 0;
    virtual void Send(const char* msg) = 0;
};

// 创建IPC实例
ASYNCIPC_API int CreateInstance(PipeImplType name, INamedPipe** pipe);
// 初始化日志库
ASYNCIPC_API void InitEasyLog(const char* process_name);