#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Renderer2
{
public:
	Renderer2(
		ID3D12Device * dev,
		ID3D12GraphicsCommandList * cmdList,
		IDXGISwapChain4 * swapchain,
		ID3D12CommandAllocator * cmdAllocator,
		ID3D12CommandQueue * cmdQueue,
		ID3D12DescriptorHeap * cbvHeap,
		UINT rtvDescSize,
		ID3D12Resource * renderTargets[2]
	);
	void Init2();
	
	void Draw2();

private:
	ID3D12DescriptorHeap* _cbvHeap;
};


