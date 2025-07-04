#include "frameResource.h"

#include "bufferManager.h"
#include "directx12Util.h"
#include "light.h"
#include "material.h"
#include "uploadBuffer.h"

// --------------------------------------------------------------------------------------------------------------------------

sFrameResource::sFrameResource(ID3D12Device* _pDevice, UINT _passCount, UINT _objectCount, UINT _directionalLightCount)
	: fence(0)
	, pCmdListAlloc(nullptr)
	, pObjectCB(nullptr)
	, pPassCB(nullptr)
	, pDirectLightCB(nullptr)
	, pMaterialCB(nullptr)
{
	cDirectX12Util::ThrowIfFailed(_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(pCmdListAlloc.GetAddressOf())
	));

	pObjectCB		= new cUploadBuffer<sObjectConstants>(_pDevice, _objectCount, true);
	pPassCB			= new cUploadBuffer<sPassConstants>(_pDevice, _passCount, true);
	pDirectLightCB	= new cUploadBuffer<sLightConstants>(_pDevice, _directionalLightCount, true);
}

// --------------------------------------------------------------------------------------------------------------------------

sFrameResource::~sFrameResource()
{
	delete pObjectCB;
	delete pPassCB;
	delete pDirectLightCB;
}

// --------------------------------------------------------------------------------------------------------------------------
