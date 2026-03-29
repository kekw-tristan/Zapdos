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