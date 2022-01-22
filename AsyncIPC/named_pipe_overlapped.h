#pragma once
#include "async_ipc.h"
#include "named_pipe_impl.h"

class NamedPipeOverlapped
    : public NamedPipeImpl
{
public:
    NamedPipeOverlapped();
    ~NamedPipeOverlapped();
    bool Create(LPCWSTR pipe_name, PipeType type, IPipeDelegate* delegate) override;
    void Exit() override;

protected:
    bool PipeWrite(void* data, uint32_t size) override;
    bool PipeRead() override;
    void DoWork() override;

    bool OnReadComplete();
    bool OnWriteComplete();

private:
    int recv_flag_;
    int send_flag_;
    PipeOverlapped overlappeds_[2];
    std::string recv_buffer_;
};

