#include "3DRenderer.h"
#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")
#include <DirectXMath.h>


Renderer2::Renderer2(
	ID3D12Device* dev,
	ID3D12GraphicsCommandList* cmdList,
	IDXGISwapChain4* swapchain,
	ID3D12CommandAllocator* cmdAllocator,
	ID3D12CommandQueue* cmdQueue,
	ID3D12DescriptorHeap* cbvHeap,
	UINT rtvDescSize,
	ID3D12Resource* renderTargets[2]
)
{
	// Constructor implementation (if needed)
}

void Renderer2::Init2()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ID3D12DescriptorHeap* cbvHeap;
	_cbvHeap>CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap));
}