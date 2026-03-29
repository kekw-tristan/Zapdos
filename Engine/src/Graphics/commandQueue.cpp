#include "commandQueue.h"

#include <iostream>

#include "directx12Util.h"

// --------------------------------------------------------------------------------------------------------------------------

cCommandQueue::cCommandQueue()
	:m_pCommandQueue(nullptr)
	,m_pFence(nullptr)
	,m_nextFenceValue(1)
	,m_fenceEvent(nullptr)
{
}

// --------------------------------------------------------------------------------------------------------------------------

cCommandQueue::~cCommandQueue()
{
	if (m_fenceEvent != nullptr)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandQueue::Initialize(ID3D12Device* _pDevice, D3D12_COMMAND_LIST_TYPE _type)
{
	// create fence
	cDirectX12Util::ThrowIfFailed(_pDevice->CreateFence(
		0,                          
		D3D12_FENCE_FLAG_NONE,      
		IID_PPV_ARGS(&m_pFence)     
	));


	// create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {}; 

	queueDesc.Type		= _type; 
	queueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE; 
	queueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; 
	queueDesc.NodeMask	= 0;

	cDirectX12Util::ThrowIfFailed(_pDevice->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(m_pCommandQueue.GetAddressOf())
	)); 

	// create Event
	m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS); 
	if (m_fenceEvent == nullptr)
	{
		throw std::runtime_error("Failed to create fence event");
	}

}

// --------------------------------------------------------------------------------------------------------------------------

UINT64 cCommandQueue::Execute(ID3D12CommandList** _ppLists, UINT _listCount)
{
	if (_ppLists == nullptr || _listCount == 0)
	{
		throw std::runtime_error("Excecute called with no command lists"); 
	}

	m_pCommandQueue->ExecuteCommandLists(_listCount, _ppLists); 


	return Signal();
}

// --------------------------------------------------------------------------------------------------------------------------

UINT64 cCommandQueue::Signal()
{
	const UINT64 fenceValue = m_nextFenceValue; 
	cDirectX12Util::ThrowIfFailed(
		m_pCommandQueue->Signal(m_pFence.Get(), fenceValue)
	); 
	++m_nextFenceValue; 
	return fenceValue;
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandQueue::WaitCPU(UINT64 _fenceValue)
{
	if (m_pFence->GetCompletedValue() >= _fenceValue)
	{
		return; 
	}

	cDirectX12Util::ThrowIfFailed(
		m_pFence->SetEventOnCompletion(_fenceValue, m_fenceEvent)
	);

	WaitForSingleObject(m_fenceEvent, INFINITE);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCommandQueue::Flush()
{
	const UINT64 fenceValue = Signal(); 
	WaitCPU(fenceValue);
}

// --------------------------------------------------------------------------------------------------------------------------

UINT64 cCommandQueue::GetCompletedValue() const
{
	return UINT64();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12CommandQueue* cCommandQueue::GetCommandQueue()const
{
	return m_pCommandQueue.Get();
}

// --------------------------------------------------------------------------------------------------------------------------
