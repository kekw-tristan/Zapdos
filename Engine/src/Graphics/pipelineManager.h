#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>

using namespace Microsoft::WRL;

class cDeviceManager;

class cPipelineManager
{
	public:

		cPipelineManager(cDeviceManager* _pDeviceManager);

	public:

		void Initialize();

	public:

		ID3D12PipelineState* GetPipelineStateObject() const;
		ID3D12RootSignature* GetRootSignature() const;

	private:

		void InitializeRootSignature();
		void InitializeShader();
		void InitializePipelineStateObject();

	private:


		cDeviceManager* m_pDeviceManager; 

		ComPtr<ID3D12RootSignature> m_pRootSignature;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;

		ComPtr<ID3DBlob> m_pVsByteCode;
		ComPtr<ID3DBlob> m_pPsByteCode;

		ComPtr<ID3D12PipelineState> m_pPso;
};