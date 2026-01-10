#include "Renderer.h"
#include <d3dcompiler.h>
#include <Windows.h>
#pragma comment(lib,"d3dcompiler.lib")

struct Vertex { float pos[3]; float col[4]; };

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
    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.Descriptor.ShaderRegister = 0;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = &rootParam;
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
cbuffer ConstBuffer : register(b0)
{
    float4x4 mat;
};

PSInput VS(VSInput input)
{
    PSInput o;
    o.pos = mul(float4(input.pos, 0, 1), mat);
    o.col = input.col;
    return o;
}
float4 PS(PSInput input) : SV_TARGET { return float4(1,0,0,1); }
)";
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* err = nullptr;
    D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vsBlob, &err);
    D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, &psBlob, &err);

    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    ZeroMemory(&psoDesc, sizeof(psoDesc));
    // 入力レイアウト
    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };

    psoDesc.InputLayout = { inputElements, _countof(inputElements) };


    // 入力レイアウト
    psoDesc.InputLayout = { inputElements, _countof(inputElements) };
    psoDesc.pRootSignature = _rootSig;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    // 三角形描画
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // --- RASTERIZER  ---
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // --- BLEND  ---
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rtBlend{};
    rtBlend.BlendEnable = FALSE;
    rtBlend.LogicOpEnable = FALSE;
    rtBlend.SrcBlend = D3D12_BLEND_ONE;
    rtBlend.DestBlend = D3D12_BLEND_ZERO;
    rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
    rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.BlendState.RenderTarget[0] = rtBlend;

    // --- DEPTH STENCIL ---
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    // --- フォーマット・サンプル ---
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.NumRenderTargets = 1;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;

    // 作成
    _dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pso));


    // 頂点バッファ  四角形
    Vertex vertices[] = {
        {{-0.25f,-0.5f,0},{0,0,0,0}},
        {{-0.25f,0.5f,0},{0,0,0,0}},
        {{0.25f,-0.5f,0},{0,0,0,0}},
        {{0.25f,0.5f,0},{0,0,0,0}}
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
    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC cbDesc{};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = (sizeof(ConstBufferData) + 255) & ~255; // 256byte alignment
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    _dev->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_constantBuffer)
    );

    
    _constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&_cbvMappedData));

    _dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
    _fenceValue = 1;
    _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void Renderer::Draw()
{
    _frameIndex = _swapchain->GetCurrentBackBufferIndex();
    _cmdAllocator->Reset();
    _cmdList->Reset(_cmdAllocator, _pso);

   

    
    D3D12_RESOURCE_BARRIER barrier_to_rt{};
    barrier_to_rt.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_to_rt.Transition.pResource = _renderTargets[_frameIndex];
    barrier_to_rt.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;        
    barrier_to_rt.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;  
    _cmdList->ResourceBarrier(1, &barrier_to_rt);

    

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += _frameIndex * _rtvDescSize;
    FLOAT clearColor[] = { 0,0,0,1 };
    _cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    _cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    _cmdList->SetGraphicsRootSignature(_rootSig);
    _cmdList->SetGraphicsRootConstantBufferView(
        0, _constantBuffer->GetGPUVirtualAddress());
    _cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); //D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELISTは三角
    _cmdList->IASetVertexBuffers(0, 1, &_vbView);
    

    
    D3D12_VIEWPORT viewport{};
    viewport.Width = 1280.0f;   
    viewport.Height = 720.0f;
    viewport.MaxDepth = 1.0f;
    _cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect{};
    scissorRect.right = 1280;
    scissorRect.bottom = 720;
    _cmdList->RSSetScissorRects(1, &scissorRect);
    _cmdList->DrawInstanced(4, 1, 0, 0);

    

    
    D3D12_RESOURCE_BARRIER barrier_to_present{};
    barrier_to_present.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_to_present.Transition.pResource = _renderTargets[_frameIndex];
    barrier_to_present.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; 
    barrier_to_present.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;       
    _cmdList->ResourceBarrier(1, &barrier_to_present);

    _cmdList->Close();
    ID3D12CommandList* lists[] = { _cmdList };
    _cmdQueue->ExecuteCommandLists(1, lists);
    _swapchain->Present(1, 0);
    // GPU にフェンスを投げる
    _cmdQueue->Signal(_fence, _fenceValue);

    // GPU が終わるまで待つ
    if (_fence->GetCompletedValue() < _fenceValue)
    {
        _fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
        WaitForSingleObject(_fenceEvent, INFINITE);
    }

    _fenceValue++;

}
void Renderer::Update()
{
    float speed = 0.004f;

    if (GetAsyncKeyState('W') & 0x8000) posY += speed;
    if (GetAsyncKeyState('S') & 0x8000) posY -= speed;
    if (GetAsyncKeyState('A') & 0x8000) posX -= speed;
    if (GetAsyncKeyState('D') & 0x8000) posX += speed;

    if (GetAsyncKeyState('Q') & 0x8000) scale += 0.003f;
    if (GetAsyncKeyState('E') & 0x8000) scale -= 0.003f;
    scale = max(scale, 0.1f);

    using namespace DirectX;
    XMMATRIX S = XMMatrixScaling(scale, scale, 1.0f);
    XMMATRIX T = XMMatrixTranslation(posX, posY, 0.0f);

    XMMATRIX world = S * T;

    ConstBufferData cb{};
    cb.mat = XMMatrixTranspose(world);
    memcpy(_cbvMappedData, &cb, sizeof(cb));
}