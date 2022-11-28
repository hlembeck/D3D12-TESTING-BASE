#pragma once
#include "DXInstance.h"

class Game : public DXInstance {
public:
	Game(UINT width, UINT height);
	~Game();

	void OnInit();
	void OnUpdate();
	void OnRender();
	void OnDestroy();

	void LoadGraphicsPipeline();
	void LoadAssets();
private:
	void WaitForPreviousFrame();
	void FillCommandList();
	void CreateRootSignature();
	void CreatePipelineState();

	static const UINT NUMFRAMES = 2;
	//ID3D12
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12Resource> m_renderTargets[NUMFRAMES];
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	UINT m_rtvDescriptorSize;


	//IDXGI
	ComPtr<IDXGISwapChain4> m_swapChain;

	//Synchronization
	UINT m_frameIndex;
	UINT64 m_fenceValue;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
};