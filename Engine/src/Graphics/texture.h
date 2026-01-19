#pragma once

#include <string>
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <d3dx12.h>

using namespace Microsoft::WRL;

class cTexture {

	public:

		cTexture(int _index, std::wstring& _rFilePath); 

	public:

		void LoadTexture(ID3D12Device* _pDevice);
		void UploadToGpu(ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList);
		ID3D12Resource* GetResource();
		int GetIndex();
		int GetMipLevels();
		std::wstring& GetFilePath();

	private:

		int			m_index; 
		std::wstring m_filePath; 

		ComPtr<ID3D12Resource> m_pResource;
		ComPtr<ID3D12Resource> m_pUploadHeap;

		D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle; 

		std::unique_ptr<uint8_t[]> m_pDdsData;
		std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;

		int m_width;
		int m_height;
		int m_mipLevels;
};