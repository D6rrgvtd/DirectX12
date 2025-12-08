#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tchar.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#include <iostream>
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
ID3D12Fence* _fence = nullptr;

ID3D12DescriptorHeap* _rtvHeap = nullptr;
UINT _rtvDescriptorSize = 0;
ID3D12Resource* _renderTargets[2];

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

    // --- Swap Chain ---
    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = window_width;
    scDesc.Height = window_height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferCount = 2;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain1* swapchain1 = nullptr;
    hr = _dxgiFactory->CreateSwapChainForHwnd(
        _cmdQueue,
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &swapchain1
    );
    if (FAILED(hr)) return -1;

    swapchain1->QueryInterface(IID_PPV_ARGS(&_swapchain));
    swapchain1->Release();

    // --- RTV Heap ---
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = 2;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = _dev->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&_rtvHeap));
    if (FAILED(hr)) return -1;

    _rtvDescriptorSize =
        _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        _rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < 2; i++)
    {
        hr = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
        if (FAILED(hr)) return -1;

        _dev->CreateRenderTargetView(_renderTargets[i], nullptr, rtvHandle);
        
        rtvHandle.ptr += _rtvDescriptorSize;

    }
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

    _dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));


    
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

