#pragma once
#include "async_ipc.h"
#include "named_pipe_define.h"

#include <thread>
#include <mutex>
#include <string>
#include <list>
#include <memory>
#include <string>
#include <windows.h>
#include "named_pipe_impl.h"

class IPipeDelegate;
class NamedPipeAsync
    : public NamedPipeImpl
{
public:
    NamedPipeAsync();
    ~NamedPipeAsync();

protected:

    bool PipeWrite(void* data, uint32_t size) override;
    bool PipeRead() override;

    static void WINAPI OnWriteCompletionRoutine(
        DWORD dwErrorCode,
        DWORD dwNumberOfBytesTransfered,
        LPOVERLAPPED lpOverlapped
        );

    static void WINAPI OnReadCompletionRoutine(
        DWORD dwErrorCode,
        DWORD dwNumberOfBytesTransfered,
        LPOVERLAPPED lpOverlapped
        );

    void DoWork();

private:
    std::string recv_buffer_;
};

