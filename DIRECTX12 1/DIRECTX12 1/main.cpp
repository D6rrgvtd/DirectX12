#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tchar.h>

#ifdef _DEBUG
#include <iostream>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#endif

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

// ===== グローバル DX12 オブジェクト =====
ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main()
{
    DebugOutputFormatString("Show window test.\n");
    getchar();

    // ======== ウィンドウ作成 ========
    WNDCLASSEX w = {};
    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = WindowProcedure;
    w.lpszClassName = _T("DX12");
    w.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&w);

    RECT wrc = { 0,0, window_width, window_height };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    HWND hwnd = CreateWindow(
        w.lpszClassName,
        _T("DX12"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr, nullptr, w.hInstance, nullptr
    );

    ShowWindow(hwnd, SW_SHOW);

    // ================================================
    // ▼ ここから DirectX12 初期化
    // ================================================

    HRESULT hr;

    // --- DXGI Factory ---
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
    if (FAILED(hr)) { MessageBoxA(0, "DXGI Factory Error", "ERR", 0); return -1; }

    // --- Device ---
    hr = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&_dev)
    );
    if (FAILED(hr)) { MessageBoxA(0, "Device Error", "ERR", 0); return -1; }

    // --- Command Queue ---
    D3D12_COMMAND_QUEUE_DESC qDesc = {};
    qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = _dev->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&_cmdQueue));
    if (FAILED(hr)) { MessageBoxA(0, "CmdQueue Error", "ERR", 0); return -1; }

    // --- Command Allocator ---
    hr = _dev->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&_cmdAllocator)
    );
    if (FAILED(hr)) { MessageBoxA(0, "CmdAllocator Error", "ERR", 0); return -1; }

    // --- Command List ---
    hr = _dev->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        _cmdAllocator,
        nullptr,
        IID_PPV_ARGS(&_cmdList)
    );
    if (FAILED(hr)) { MessageBoxA(0, "CmdList Error", "ERR", 0); return -1; }

    _cmdList->Close();

    // ================================================
    // ▲ DirectX12 初期化 ここまで
    // ================================================


    // ======== メッセージループ ========
    MSG msg = {};

    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
            break;
    }

    UnregisterClass(w.lpszClassName, w.hInstance);
    return 0;
}
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    MessageBoxA(NULL, "Show window test.", "Info", MB_OK);
    return 0;
}
#endif

