#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>

struct Bullet
{
    bool active = false;
    DirectX::XMFLOAT2 pos;
    DirectX::XMFLOAT2 vel;
    float radius = 0.03f;
};

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
    float scale = 0.7f;
    void Init();
    void Draw();
    void Update();

private:
    ID3D12Device* _dev;
    ID3D12GraphicsCommandList* _cmdList;
    IDXGISwapChain4* _swapchain;
    ID3D12CommandAllocator* _cmdAllocator;
    ID3D12CommandQueue* _cmdQueue;
    ID3D12DescriptorHeap* _rtvHeap;
    UINT _rtvDescSize;
    ID3D12Resource* _renderTargets[2];
    ID3D12Fence* _fence = nullptr;
    UINT64 _fenceValue = 0;
    HANDLE _fenceEvent = nullptr;
    ID3D12RootSignature* _rootSig = nullptr;
    ID3D12PipelineState* _pso = nullptr;
    ID3D12Resource* _vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW _vbView = {};
    ID3D12Resource* _triangleVB = nullptr;
    D3D12_VERTEX_BUFFER_VIEW _trianglevbView = {};
    std::vector<Bullet> _bullets;
    float _fireTimer = 0.0f;
    ID3D12Resource* _bulletVB = nullptr;
    D3D12_VERTEX_BUFFER_VIEW _bulletVBView{};
    ID3D12Resource* _bulletCB = nullptr;
    void* _bulletCBData = nullptr;

    ID3D12Resource* _triangleCB = nullptr;
    UINT8* _triangleCBData = nullptr;

    ID3D12Resource* _rectCB = nullptr;
    UINT8* _rectCBData = nullptr;
    
};
