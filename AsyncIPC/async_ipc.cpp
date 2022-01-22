// AsyncIPC.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "async_ipc.h"
#include <shlobj.h>
#include "named_pipe_async.h"
#include "named_pipe_overlapped.h"

BOOL GetExePathA(char* path, uint32_t size)
{
    if (path == nullptr || size < 64) {
        return FALSE;
    }
    ::GetModuleFileNameA(NULL, path, size);
    int length = strlen(path);
    for (int i = 0; i < length; ++i) {
        if (path[length - 1 - i] == '\\') {
            break;
        }
        path[length - 1 - i] = '\0';
    }
    return TRUE;
}

int CreateInstance(PipeImplType name, INamedPipe** pipe)
{
    INamedPipe* pipe_impl = nullptr;
    switch (name)
    {
    case PIPE_ASYNC:
        pipe_impl = new NamedPipeAsync;
        break;
    case PIPE_OVERLAPPED:
        pipe_impl = new NamedPipeOverlapped;
        break;
    default:
        break;
    }
    *pipe = pipe_impl;
    return 0;
}

void InitEasyLog(const char* process_name)
{
    std::string format;
    format = "%datetime %level [A] %fbase:%line %msg ";
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Format, format);
    defaultConf.set(el::Level::Warning,
        el::ConfigurationType::Format, format);


    char log_path[MAX_PATH] = { 0 };
    GetExePathA(log_path, MAX_PATH);
    strcat_s(log_path, "\\log\\");
    SHCreateDirectoryExA(NULL, log_path, NULL);
    SYSTEMTIME stCur;
    ::GetLocalTime(&stCur);
    char current_date[128] = { 0 };
    sprintf_s(current_date, 128, "AsyncIPC_%s_%04d-%02d-%02d.log", process_name, stCur.wYear,
        stCur.wMonth, stCur.wDay);
    strcat_s(log_path, current_date);
    std::string log_file(log_path);

    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Warning,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Global,
        el::ConfigurationType::Filename, log_file);
    defaultConf.set(el::Level::Error,
        el::ConfigurationType::Filename, log_file);
    el::Loggers::reconfigureLogger("default", defaultConf);
}