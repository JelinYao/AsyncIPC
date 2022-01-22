# AsyncIPC
Windows上高性能异步IPC实现（基于命名管道），支持异步双向通信，接口简洁使用简单。
A high-performance asynchronous IPC implementation on Windows (based on named pipes),Support asynchronous two-way communication, the interface is concise and easy to use.

## 实现介绍 
异步IPC框架提供2套实现方案，都是基于Windows系统的能力，分别是异步过程调用和重叠I/O。

###  1.异步过程调用（Asynchronous Procedure Calls） 
MSDN：https://docs.microsoft.com/zh-cn/windows/win32/sync/asynchronous-procedure-calls 

基于异步过程调用实现的命名管道：https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-completion-routines

### 2.重叠I/O
基于重叠I/O实现的命名管道：https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-overlapped-i-o

## 接口介绍 
### 1.导出接口 
全部以C接口形式导出，便于其他语言调用。 
```C++
// 创建IPC实例
ASYNCIPC_API int CreateInstance(PipeImplType name, INamedPipe** pipe);
// 初始化日志库
ASYNCIPC_API void InitEasyLog(const char* process_name);
```
### 2.IPC接口类 
提供IPC创建、发送消息、退出等功能。
```C++
class INamedPipe
{
public:
    virtual bool Create(const wchar_t* pipe_name, PipeType type, IPipeDelegate* delegate) = 0;
    virtual void Exit() = 0;
    virtual void Send(const char* msg) = 0;
};
```
### 3.IPC回调接口类 
将IPC内部状态信息回调到接入方，包括数据发送、消息接收、连接状态等信息。
```C++
class IPipeDelegate {
public:
    virtual void OnCreate(bool) = 0;
    virtual void OnConnected(bool) = 0;
    virtual void OnSend(int size) = 0;
    virtual void OnRecv(void* data, int size) = 0;
    virtual void OnDisconnected() = 0;
};
```
## 使用介绍 
### 1.server端
示例代码参考：AsyncIPCServer项目，实现在AsyncIPC\AsyncIPCServer\main.cpp中。
```c++
#define ENABLE_ASYNCPIPE
INamedPipe* pipe_server_ = nullptr;
...
PipeCallback pipe_clllback;
    PipeImplType name;
#ifdef ENABLE_ASYNCPIPE
    name = PIPE_ASYNC;
#else
    name = PIPE_OVERLAPPED;
#endif
    CreateInstance(name, &pipe_server_);
    pipe_server_->Create(kPipeName, PIPE_SERVER, &pipe_clllback);
    ...
    pipe_server_->Send(text);
    ...
    pipe_server_->Exit();
```

### 2.client端
示例代码参考：AsyncIPCClient项目，实现在AsyncIPC\AsyncIPCClient\main.cpp中。
```c++
#define ENABLE_ASYNCPIPE
INamedPipe* pipe_client_ = nullptr;
PipeCallback pipe_clllback;
    PipeImplType name;
#ifdef ENABLE_ASYNCPIPE
    name = PIPE_ASYNC;
#else
    name = PIPE_OVERLAPPED;
#endif
    CreateInstance(name, &pipe_client_);
    pipe_client_->Create(kPipeName, PIPE_CLIENT, &pipe_clllback);

// send msg
pipe_client_->Send(text);
// exit
pipe_client_->Exit();
```
## 测试截图 

1.server和client建立连接 

![](https://raw.githubusercontent.com/JelinYao/AsyncIPC/main/img/connect.png)

2.server和client之间互相发送消息 

![](https://raw.githubusercontent.com/JelinYao/AsyncIPC/main/img/msg.png)

3.断开连接 

![](https://raw.githubusercontent.com/JelinYao/AsyncIPC/main/img/disconnect1.png) 

![](https://raw.githubusercontent.com/JelinYao/AsyncIPC/main/img/disconnect2.png)