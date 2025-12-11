#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <tchar.h>
#include "Renderer.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const UINT width = 1280, height = 720;
    WNDCLASSEX w{}; w.cbSize = sizeof(WNDCLASSEX); w.lpfnWndProc = DefWindowProc; w.lpszClassName = _T("DX12"); w.hInstance = hInstance;
    RegisterClassEx(&w);
    HWND hwnd = CreateWindow(w.lpszClassName, _T("DX12 Triangle"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);

    IDXGIFactory6* dxgiFactory = nullptr;
    CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    ID3D12Device* dev = nullptr; D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev));
    ID3D12CommandQueue* cmdQueue = nullptr; D3D12_COMMAND_QUEUE_DESC q{}; q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; dev->CreateCommandQueue(&q, IID_PPV_ARGS(&cmdQueue));
    ID3D12CommandAllocator* cmdAllocator = nullptr; dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
    ID3D12GraphicsCommandList* cmdList = nullptr; dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList)); cmdList->Close();
    IDXGISwapChain4* swapchain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 sc{}; sc.Width = width; sc.Height = height; sc.BufferCount = 2; sc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; sc.SampleDesc.Count = 1;
    IDXGISwapChain1* swap1 = nullptr; dxgiFactory->CreateSwapChainForHwnd(cmdQueue, hwnd, &sc, nullptr, nullptr, &swap1);
    swap1->QueryInterface(IID_PPV_ARGS(&swapchain)); swap1->Release();

    ID3D12DescriptorHeap* rtvHeap = nullptr; D3D12_DESCRIPTOR_HEAP_DESC rtv{}; rtv.NumDescriptors = 2; rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; dev->CreateDescriptorHeap(&rtv, IID_PPV_ARGS(&rtvHeap));
    UINT rtvDescSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    ID3D12Resource* rtvBuffers[2]; D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0;i < 2;i++) { swapchain->GetBuffer(i, IID_PPV_ARGS(&rtvBuffers[i])); dev->CreateRenderTargetView(rtvBuffers[i], nullptr, handle); handle.ptr += rtvDescSize; }

    Renderer renderer(dev, cmdList, swapchain, cmdAllocator, cmdQueue, rtvHeap, rtvDescSize, rtvBuffers);
    renderer.Init();

    MSG msg{}; while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); if (msg.message == WM_QUIT) break; }
        renderer.Draw();
    }

    return 0;
}
