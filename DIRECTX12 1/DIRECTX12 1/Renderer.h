#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Renderer
{
public:
    Renderer(
        ID3D12Device* dev,
        ID3D12GraphicsCommandList* cmdList,
        IDXGISwapChain4* swapchain,
        ID3D12CommandAllocator* cmdAllocator,
        ID3D12CommandQueue* cmdQueue,
        ID3D12DescriptorHeap* rtvHeap,
        UINT rtvDescSize,
        ID3D12Resource* renderTargets[2]
    );

    void Init();   // éOäpå`ï`âÊÇÃèÄîı
    void Draw();   // ñàÉtÉåÅ[ÉÄÇÃï`âÊ

private:
    ID3D12Device* _dev = nullptr;
    ID3D12GraphicsCommandList* _cmdList = nullptr;
    IDXGISwapChain4* _swapchain = nullptr;
    ID3D12CommandAllocator* _cmdAllocator = nullptr;
    ID3D12CommandQueue* _cmdQueue = nullptr;
    ID3D12DescriptorHeap* _rtvHeap = nullptr;
    UINT _rtvDescSize = 0;
    ID3D12Resource* _renderTargets[2]{ nullptr, nullptr };

    // éOäpå`ï`âÊóp
    ID3D12Resource* _vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW _vbView{};

    ID3D12PipelineState* _pso = nullptr;
    ID3D12RootSignature* _rootSig = nullptr;

    UINT _frameIndex = 0;
};
