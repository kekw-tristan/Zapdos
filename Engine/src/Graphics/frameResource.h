#pragma once

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

template <typename T>
class cUploadBuffer;

struct sObjectConstants;
//struct sPassConstants;

struct cFrameResource
{

	public:

		cFrameResource(ID3D12Device* _pDevice, UINT _passCount, UINT _objectCount);
		~cFrameResource();

	public:

		ComPtr<ID3D12CommandAllocator> m_pCmdListAlloc;

		cUploadBuffer<sObjectConstants>*	m_ObjectCB;
		//cUploadBuffer<sPassConstants>*		m_pPassCB;
		
		UINT64 m_fence;
};
