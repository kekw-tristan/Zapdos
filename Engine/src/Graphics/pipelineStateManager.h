#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>

class cShaderManager; 
class cRootSignatureManager;

using namespace Microsoft::WRL;

class cPipelineStateManager
{
	public:
	
		cPipelineStateManager();
		~cPipelineStateManager();

	public:

		void Initialize(ID3D12Device* _pDevice, cShaderManager* _pShaderManager, cRootSignatureManager* _pRootSignatureManager); 

		ID3D12PipelineState* GetPipelineState(const std::string& _rName);


	private:

		void CreateGraphicsPSO(); 
		void CreateMipGenPso();

	private:
		
		ID3D12Device*			m_pDevice;
		cShaderManager*			m_pShaderManager;
		cRootSignatureManager*	m_pRootSignatureManager;

		std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_pipelineStateObjects;
};