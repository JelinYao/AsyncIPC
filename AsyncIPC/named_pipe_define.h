#pragma once
#include <windows.h>

enum OverlappedType {
    OverlappedRead = 0,
    OverlappedWrite,
};

static const int kPipeTimeout = 5000;

static const int kPipeBufferSize = 4096;

struct PipeOverlapped {
    OVERLAPPED ol;
    void* param;
    char buffer[kPipeBufferSize + 1];
    unsigned char type;
    DWORD trans_bytes;
};

static const int kOverlappedSize = sizeof(PIPE_OVERLAPPED);