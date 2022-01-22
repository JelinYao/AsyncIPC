// AsyncIPCClient.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "AsyncIPCClient.h"
#include <ShlObj.h>
#include "../AsyncIPC/async_ipc.h"
#include "Resource.h"
#include <stdio.h>
#ifdef _DEBUG
#pragma comment(lib, "../Debug/AsyncIPC.lib")
#else
#pragma comment(lib, "../Release/AsyncIPC.lib")
#endif

#define MAX_LOADSTRING 100
static const wchar_t* kPipeName = L"\\\\.\\pipe\\6CE36942-970D-4E4E-AEA7-F401A8A36242";

// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
#define ENABLE_ASYNCPIPE
INamedPipe* pipe_client_ = nullptr;

HWND wnd_edit = NULL;
HWND wnd_btn = NULL;
HWND wnd_listbox = NULL;

// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

class PipeCallback
    : public IPipeDelegate {
public:
    void OnCreate(bool result) override {

    }
    void OnConnected(bool result) override {
        char msg[32] = { 0 };
        sprintf_s(msg, "Connected %s", result ? "" : "");
        ::SendMessageA(wnd_listbox, LB_ADDSTRING, 0, (LPARAM)msg);
    }
    void OnSend(int size) override {
        char msg[32] = { 0 };
        sprintf_s(msg, "Send msg: %d bytes transfer", size);
        ::SendMessageA(wnd_listbox, LB_ADDSTRING, 0, (LPARAM)msg);
    }
    void OnRecv(void* data, int size) override {
        char msg[256] = { 0 };
        strcat_s(msg, "Recv msg: ");
        strncat_s(msg, 256, (const char*)data, size);
        ::SendMessageA(wnd_listbox, LB_ADDSTRING, 0, (LPARAM)msg);
    }
    void OnDisconnected() override {
        char* msg = "Pipe disconnected";
        ::SendMessageA(wnd_listbox, LB_ADDSTRING, 0, (LPARAM)msg);
    }
};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
    InitEasyLog("client");
    // LOG(INFO) << "AsyncIPC client start";
 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_ASYNCIPCCLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
    PipeCallback pipe_clllback;
    PipeImplType name;
#ifdef ENABLE_ASYNCPIPE
    name = PIPE_ASYNC;
#else
    name = PIPE_OVERLAPPED;
#endif
    CreateInstance(name, &pipe_client_);
    pipe_client_->Create(kPipeName, PIPE_CLIENT, &pipe_clllback);
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ASYNCIPCCLIENT));

	// 主消息循环: 
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASYNCIPCCLIENT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_ASYNCIPCCLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   int width = 420;
   int height = 300;
   int x = (::GetSystemMetrics(SM_CXSCREEN) - width) / 2;
   int y = (::GetSystemMetrics(SM_CYSCREEN) - height) / 2;
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW^WS_MAXIMIZEBOX,
      x, y, width, height, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   wnd_edit = ::CreateWindowEx(0, L"edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 30, 20, 300, 24, hWnd, (HMENU)IDC_EDIT_SEND, hInstance, NULL);
   wnd_btn = ::CreateWindowEx(0, L"button", L"发送", WS_VISIBLE | WS_CHILD | WS_BORDER, 336, 20, 42, 24, hWnd, (HMENU)IDC_BUTTON_SEND, hInstance, NULL);
   wnd_listbox = ::CreateWindowEx(0, L"listbox", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP,
       0, 60, width, height - 60, hWnd, (HMENU)IDC_LISTBOX1, hInstance, NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
        case IDC_BUTTON_SEND: {
            if (wmEvent == BN_CLICKED) {
                char text[128] = { 0 };
                ::GetDlgItemTextA(hWnd, IDC_EDIT_SEND, text, 128);
                if (strlen(text) == 0) {
                    break;
                }
                pipe_client_->Send(text);
            }
            break;
        }
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
        pipe_client_->Exit();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}