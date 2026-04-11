#include "commandContext.h"

#include <d3dx12.h>
#include <iostream>

#include "directx12Util.h"

// --------------------------------------------------------------------------------------------------------------------------

cCommandContext::cCommandContext()
	: m_pCommandList(nullptr)
{
}

// --------------------------------------------------------------------------------------------------------------------------

cCommandContext::~cCommandContext()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::Initialize(ID3D12Device* _pDeivice, ID3D12CommandAllocator* _pCmdAlloc)
{
	if (_pDeivice == nullptr)
	{
		throw std::runtime_error("cCommandText::Initialize pDeive is null");
	}

	cDirectX12Util::ThrowIfFailed(_pDeivice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		_pCmdAlloc,
		nullptr,
		IID_PPV_ARGS(m_pCommandList.GetAddressOf())
	));

	cDirectX12Util::ThrowIfFailed(m_pCommandList->Close());
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::Reset(ID3D12CommandAllocator* _pAllocator, ID3D12PipelineState* _pPso)
{
	if (_pAllocator == nullptr)
	{
		throw std::runtime_error("cCommandContext::Reset: pAllocator is null");
	}

	cDirectX12Util::ThrowIfFailed(
		m_pCommandList->Reset(_pAllocator, _pPso)
	);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::Close()
{
	cDirectX12Util::ThrowIfFailed(m_pCommandList->Close());
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12GraphicsCommandList* cCommandContext::GetCommandList() const
{
	return m_pCommandList.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::Transition(ID3D12Resource* _pResource, D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after)
{
	if (_pResource == nullptr)
	{
		throw std::runtime_error("cCommandContext::Transition: pResource is null.");
	}
	if (_before == _after)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_pResource,
		_before,
		_after
	); 

	m_pCommandList->ResourceBarrier(1, &barrier);

}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetDescriptorHeaps(UINT _count, ID3D12DescriptorHeap** _ppHeaps)
{
	m_pCommandList->SetDescriptorHeaps(_count, _ppHeaps);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetGraphicsRootSignature(ID3D12RootSignature* _pRootSignature)
{
	m_pCommandList->SetGraphicsRootSignature(_pRootSignature);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetPipelineState(ID3D12PipelineState* _pPso)
{
	m_pCommandList->SetPipelineState(_pPso);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetViewports(UINT _count, D3D12_VIEWPORT* _pViewports)
{
	m_pCommandList->RSSetViewports(_count, _pViewports);	
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetScissorRects(UINT _count, D3D12_RECT* _pScissorRect)
{
	m_pCommandList->RSSetScissorRects(1, _pScissorRect);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE _currentBackbuffer, float* _pClearColor, UINT _numRects, D3D12_RECT* _pRects)
{
	m_pCommandList->ClearRenderTargetView(_currentBackbuffer, _pClearColor, _numRects, _pRects);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE _depthsStencilView, D3D12_CLEAR_FLAGS _clearFlags, float _depth, UINT8 _stencil, UINT _numRects, D3D12_RECT* _pRects)
{
	m_pCommandList->ClearDepthStencilView(_depthsStencilView, _clearFlags, _depth, _stencil, _numRects, _pRects);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetRenderTargets(UINT _numRenderTargetDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE* _pRenderTargetDescriptors, bool _rtSingleHandleToDescriptorRange, D3D12_CPU_DESCRIPTOR_HANDLE* _pDepthStencilDescriptor)
{
	m_pCommandList->OMSetRenderTargets(_numRenderTargetDescriptors, _pRenderTargetDescriptors, _rtSingleHandleToDescriptorRange, _pDepthStencilDescriptor);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetGraphicsRootDescriptorTable(UINT _RootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE _baseDescriptor)
{
	m_pCommandList->SetGraphicsRootDescriptorTable(_RootParameterIndex, _baseDescriptor);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetVertexBuffer(UINT _startSlot, UINT _numViews, D3D12_VERTEX_BUFFER_VIEW* _pVetexBufferView)
{
	m_pCommandList->IASetVertexBuffers(0, 1, _pVetexBufferView);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* _pIndexBufferView)
{
	m_pCommandList->IASetIndexBuffer(_pIndexBufferView);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology)
{
	m_pCommandList->IASetPrimitiveTopology(_primitiveTopology);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandContext::DrawIndexedInstanced(UINT _indexCountPerInstance, UINT _instanceCount, UINT _startIndexLocation, INT _baseVertexLocation, UINT _startInstanceLoation)
{
	m_pCommandList->DrawIndexedInstanced(_indexCountPerInstance, _instanceCount, _startIndexLocation, _baseVertexLocation, _startInstanceLoation);
}

// --------------------------------------------------------------------------------------------------------------------------