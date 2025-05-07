#pragma once

#include <d3d12.h>
#include <windows.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class cDirectX12Util
{
	public:

		static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList, const void* _pInitData, UINT64 _byteSize, ComPtr<ID3D12Resource> _pUploadBuffer) ;
		static UINT CalculateBufferByteSize(UINT _byzesize);
};
