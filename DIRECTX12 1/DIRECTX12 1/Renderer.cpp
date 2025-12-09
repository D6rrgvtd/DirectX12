#include "Renderer.h"
#include <d3dcompiler.h>

#pragma comment(lib,"d3dcompiler.lib")

Renderer::Renderer(
    ID3D12Device* dev,
    ID3D12GraphicsCommandList* cmdList,
    IDXGISwapChain4* swapchain,
    ID3D12CommandAllocator* cmdAllocator,
    ID3D12CommandQueue* cmdQueue,
    ID3D12DescriptorHeap* rtvHeap,
    UINT rtvDescSize,
    ID3D12Resource* renderTargets[2]
)
{
    _dev = dev;
    _cmdList = cmdList;
    _swapchain = swapchain;
    _cmdAllocator = cmdAllocator;
    _cmdQueue = cmdQueue;
    _rtvHeap = rtvHeap;
    _rtvDescSize = rtvDescSize;
    _renderTargets[0] = renderTargets[0];
    _renderTargets[1] = renderTargets[1];

    _frameIndex = _swapchain->GetCurrentBackBufferIndex();
}

void Renderer::Init()
{
    // ---------- ① ルートシグネチャ ----------
    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* sigBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    D3D12SerializeRootSignature(
        &rsDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &sigBlob,
        &errBlob
    );

    _dev->CreateRootSignature(
        0,
        sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&_rootSig)
    );

    if (sigBlob) sigBlob->Release();
    if (errBlob) errBlob->Release();

    // ---------- ② シェーダ読み込み ----------
    ID3DBlob* vs = nullptr;
    ID3DBlob* ps = nullptr;
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VS", "vs_5_0", 0, 0, &vs, nullptr);
    D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PS", "ps_5_0", 0, 0, &ps, nullptr);

    // ---------- ③ 入力レイアウト ----------
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // ---------- ④ パイプラインステート ----------
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = _rootSig;
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    // ラスタライザ
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    // ブレンド
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // 深度無効
    psoDesc.DepthStencilState.DepthEnable = FALSE;

    _dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pso));

    if (vs) vs->Release();
    if (ps) ps->Release();

    // ---------- ⑤ 三角形の頂点バッファ ----------
    struct Vertex { float pos[2]; };

    Vertex vertices[] =
    {
        { { 0.0f,  0.5f } },
        { { 0.5f, -0.5f } },
        { { -0.5f, -0.5f } },
    };

    UINT vbSize = sizeof(vertices);

    // 頂点バッファ作成(UPLOAD)
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = vbSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    _dev->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_vertexBuffer)
    );

    // データ書き込み
    void* mapped = nullptr;
    _vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, vertices, vbSize);
    _vertexBuffer->Unmap(0, nullptr);

    // VBビュー
    _vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vbView.StrideInBytes = sizeof(Vertex);
    _vbView.SizeInBytes = vbSize;
}

void Renderer::Draw()
{
    // フレーム番号更新
    _frameIndex = _swapchain->GetCurrentBackBufferIndex();

    // ----------- コマンドアロケータ / リスト リセット -----------
    _cmdAllocator->Reset();
    _cmdList->Reset(_cmdAllocator, _pso);

    // ----------- 画面クリア -----------
    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        _rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += _rtvDescSize * _frameIndex;

    FLOAT clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
    _cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    // ----------- 描画設定 -----------
    _cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    _cmdList->SetGraphicsRootSignature(_rootSig);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->IASetVertexBuffers(0, 1, &_vbView);

    // 三角形描画
    _cmdList->DrawInstanced(3, 1, 0, 0);

    _cmdList->Close();

    // ----------- 実行 -----------
    ID3D12CommandList* lists[] = { _cmdList };
    _cmdQueue->ExecuteCommandLists(1, lists);

    // ----------- 表示 -----------
    _swapchain->Present(1, 0);
}
