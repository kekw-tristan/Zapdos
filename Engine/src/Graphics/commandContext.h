#pragma once

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class cCommandContext
{
	public:
	
		cCommandContext();
		~cCommandContext();

	public:

		void Initialize(ID3D12Device* _pDeivice, ID3D12CommandAllocator* _pCmdAlloc);
		void Reset(ID3D12CommandAllocator* _pAllocator, ID3D12PipelineState* _pPso = nullptr); 
		void Close(); 

		ID3D12GraphicsCommandList* GetCommandList() const; 

		void Transition(ID3D12Resource* _pResource, D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after);

		void SetDescriptorHeaps(UINT _count, ID3D12DescriptorHeap** _ppHeaps); 
		void SetGraphicsRootSignature(ID3D12RootSignature* _pRootSignature); 
		void SetPipelineState(ID3D12PipelineState* _pPso); 

	private:
		ComPtr<ID3D12GraphicsCommandList> m_pCommandList; 
};