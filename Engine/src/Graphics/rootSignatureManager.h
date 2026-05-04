#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>

using namespace Microsoft::WRL;

class cRootSignatureManager
{
	public:

		cRootSignatureManager();
		~cRootSignatureManager();

	public:

		void Initialize();

		ID3D12RootSignature* GetRootSignature(const std::string& name);

	private:

		void CreateGraphicsRS();
		void CreateMipGenRS();

	private:

		ID3D12Device* m_pDevice;
		std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> m_rootSignatures;
};