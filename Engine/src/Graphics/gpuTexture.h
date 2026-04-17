#pragma once

#include <d3d12.h>

class cCpuTexture;

class cGpuTexture
{
	public:
		cGpuTexture();
		~cGpuTexture();
	
	public:
	
		void UploadToGpu(cCpuTexture& _rCpuTexture, ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList);
	
	public:

		ID3D12Resource* GetResource(); 
	
	private:
	
		ID3D12Resource* m_pTexture;
		ID3D12Resource* m_pUploadheap;
	
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle{};
	
};

