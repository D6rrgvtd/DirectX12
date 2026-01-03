#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

class Renderer {
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
    float posX = 0.0f;
    float posY = 0.0f;
    void Init();
    void Update();
    void Draw();

private:
    ID3D12Device* _dev;
    ID3D12GraphicsCommandList* _cmdList;
    IDXGISwapChain4* _swapchain;
    ID3D12CommandAllocator* _cmdAllocator;
    ID3D12CommandQueue* _cmdQueue;
    ID3D12DescriptorHeap* _rtvHeap;
    UINT _rtvDescSize;
    ID3D12Resource* _renderTargets[2];

    ID3D12RootSignature* _rootSig = nullptr;
    ID3D12PipelineState* _pso = nullptr;
    ID3D12Resource* _vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW _vbView = {};
    UINT _frameIndex;
    struct ConstBufferData
    {
        DirectX::XMMATRIX mat;
   };

    ID3D12Resource* _constantBuffer = nullptr;
    UINT8* _cbvMappedData = nullptr;
};
