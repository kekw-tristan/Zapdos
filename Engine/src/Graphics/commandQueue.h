#pragma once

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class cCommandQueue
{
	public:

		cCommandQueue(); 
		~cCommandQueue(); 

	public:

		void Initialize(ID3D12Device* _pDevice, D3D12_COMMAND_LIST_TYPE _type); 
		
		UINT64 Execute(ID3D12CommandList** _ppLists, UINT _listCount);
		UINT64 Signal();

		void WaitCPU(UINT64 _fenceValue); 
		void Flush();

		UINT64 GetCompletedValue() const; 

		ID3D12CommandQueue* GetCommandQueue() const; 

	private:

		ComPtr<ID3D12CommandQueue>	m_pCommandQueue; 
		ComPtr<ID3D12Fence>			m_pFence; 
		UINT64						m_nextFenceValue; 
		HANDLE						m_fenceEvent; 


};