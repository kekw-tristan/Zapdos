#pragma once

#include <d3d12.h>
#include <d3dx12.h>
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

		void SetViewports(UINT _count, D3D12_VIEWPORT* _pViewports);
		void SetScissorRects(UINT _count, D3D12_RECT* _pScissorRects);

		void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE _currentBackbuffer, float* _pClearColor, UINT _numRects = 0, D3D12_RECT* _pRects = nullptr);
		void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE _depthsStencilView, 
			D3D12_CLEAR_FLAGS _clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
			float _depth = 1.0f, UINT8 _stencil = 0, UINT _numRects = 0, D3D12_RECT* _pRects = nullptr);

		void SetRenderTargets(UINT _numRenderTargetDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE* _pRenderTargetDescriptors, bool _rtSingleHandleToDescriptorRange, D3D12_CPU_DESCRIPTOR_HANDLE* _pDepthStencilDescriptor);
		void SetGraphicsRootDescriptorTable(UINT _rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE _baseDescriptor);

		void SetVertexBuffer(UINT _startSlot, UINT _numViews, D3D12_VERTEX_BUFFER_VIEW* _pVetexBufferView);
		void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* _pIndexBufferView);
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology);

		void DrawIndexedInstanced(UINT _indexCountPerInstance, UINT _instanceCount, UINT _startIndexLocation, INT _baseVertexLocation, UINT _startInstanceLoation);
	private:
		ComPtr<ID3D12GraphicsCommandList> m_pCommandList; 
};