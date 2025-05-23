#include "frameResource.h"

#include "bufferManager.h"
#include "directx12Util.h"
#include "uploadBuffer.h"

// --------------------------------------------------------------------------------------------------------------------------

sFrameResource::sFrameResource(ID3D12Device* _pDevice, UINT _passCount, UINT _objectCount)
	: fence(0)
	, pCmdListAlloc(nullptr)
	, pObjectCB(nullptr)
{
	cDirectX12Util::ThrowIfFailed(_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(pCmdListAlloc.GetAddressOf())
	));

	// Todo: same has to be done for pass constants
	pObjectCB = new cUploadBuffer<sObjectConstants>(_pDevice, _objectCount, true);
}

// --------------------------------------------------------------------------------------------------------------------------

sFrameResource::~sFrameResource()
{
	delete pObjectCB;
}

// --------------------------------------------------------------------------------------------------------------------------
