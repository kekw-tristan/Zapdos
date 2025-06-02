#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <iostream>
#include <string>
#include <comdef.h>
#include <wrl.h>

#include "directx12Util.h"

using namespace Microsoft::WRL;

template<typename T>
class cUploadBuffer
{
	public:
		cUploadBuffer(ID3D12Device* _pDevice, UINT _elementCount, bool _isConstantBuffer)
			: m_pMappedData(nullptr)
			, m_pUploadBuffer(nullptr)
			, m_elementByteSize(0)
			, m_isConstanBuffer(_isConstantBuffer)
		{
			m_elementByteSize = sizeof(T);

			if (m_isConstanBuffer)
			{
				m_elementByteSize = cDirectX12Util::CalculateBufferByteSize(sizeof(T));
				cDirectX12Util::ThrowIfFailed(_pDevice->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_pUploadBuffer)
				));

				cDirectX12Util::ThrowIfFailed(m_pUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData)));
			}
		}

		~cUploadBuffer()
		{
			if (m_pUploadBuffer != nullptr)
				m_pUploadBuffer->Unmap(0, nullptr);
			m_pUploadBuffer = nullptr;
		}

		cUploadBuffer(const cUploadBuffer&) = delete;
		cUploadBuffer operator=(const cUploadBuffer&) = delete;

		ID3D12Resource* GetResource()
		{
			return m_pUploadBuffer.Get();
		}

		UINT GetElementByteSize()
		{
			return m_elementByteSize;
		}

		void CopyData(int _elementIndex, const T& _rData)
		{
			memcpy(&m_pMappedData[_elementIndex * m_elementByteSize], &_rData, sizeof(T));
		}

	private:
			
		BYTE* m_pMappedData;
		ComPtr<ID3D12Resource> m_pUploadBuffer;
		UINT m_elementByteSize;
		bool m_isConstanBuffer;

};

