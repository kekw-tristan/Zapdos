#pragma once

#include <string>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class cTexture {

	public:

		cTexture(int _index, std::wstring& _rFilePath); 

	public:

		void LoadTexture(ID3D12Device* _pDevice);
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

		int m_width;
		int m_height;
		int m_mipLevels;
};