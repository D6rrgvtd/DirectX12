#include "Renderer.h"
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

struct Vertex { float pos[2]; float col[3]; };

Renderer::Renderer(
    ID3D12Device* dev,
    ID3D12GraphicsCommandList* cmdList,
    IDXGISwapChain4* swapchain,
    ID3D12CommandAllocator* cmdAllocator,
    ID3D12CommandQueue* cmdQueue,
    ID3D12DescriptorHeap* rtvHeap,
    UINT rtvDescSize,
    ID3D12Resource* renderTargets[2]
) : _dev(dev), _cmdList(cmdList), _swapchain(swapchain), _cmdAllocator(cmdAllocator),
_cmdQueue(cmdQueue), _rtvHeap(rtvHeap), _rtvDescSize(rtvDescSize)
{
    _renderTargets[0] = renderTargets[0];
    _renderTargets[1] = renderTargets[1];
    _frameIndex = _swapchain->GetCurrentBackBufferIndex();
}

void Renderer::Init()
{
    // ルートシグネチャ
    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ID3DBlob* sigBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    _dev->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSig));
    if (sigBlob) sigBlob->Release();
    if (errBlob) errBlob->Release();


    // シェーダ
    const char* shaderCode = R"(
struct VSInput { float2 pos : POSITION; float3 col : COLOR; };
struct PSInput { float4 pos : SV_POSITION; float3 col : COLOR; };
PSInput VS(VSInput input) { PSInput o; o.pos = float4(input.pos,0,1); o.col = input.col; return o; }
float4 PS(PSInput input) : SV_TARGET { return float4(input.col,1); }
)";
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;
    D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &err);

    // PSO
    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = { inputElements,_countof(inputElements) };
    psoDesc.pRootSignature = _rootSig;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    _dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pso));
    if (vsBlob) vsBlob->Release();
    if (psBlob) psBlob->Release();

    // 頂点バッファ
    Vertex vertices[] = {
        {{0,0.5f},{1,0,0}},
        {{-0.5f,-0.5f},{0,1,0}},
        {{0.5f,-0.5f},{0,0,1}}
    };
    UINT vbSize = sizeof(vertices);
    D3D12_HEAP_PROPERTIES heapProp{}; heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC resDesc{}; resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = vbSize; resDesc.Height = 1; resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1; resDesc.SampleDesc.Count = 1; resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    _dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer));
    void* mapped = nullptr; _vertexBuffer->Map(0, nullptr, &mapped); memcpy(mapped, vertices, vbSize); _vertexBuffer->Unmap(0, nullptr);
    _vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vbView.StrideInBytes = sizeof(Vertex);
    _vbView.SizeInBytes = vbSize;
}

void Renderer::Draw()
{
    _frameIndex = _swapchain->GetCurrentBackBufferIndex();
    _cmdAllocator->Reset();
    _cmdList->Reset(_cmdAllocator, _pso);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += _frameIndex * _rtvDescSize;
    FLOAT clearColor[] = { 0,0,0,1 };
    _cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    _cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    _cmdList->SetGraphicsRootSignature(_rootSig);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->IASetVertexBuffers(0, 1, &_vbView);
    _cmdList->DrawInstanced(3, 1, 0, 0);

    _cmdList->Close();
    ID3D12CommandList* lists[] = { _cmdList };
    _cmdQueue->ExecuteCommandLists(1, lists);
    _swapchain->Present(1, 0);
}
