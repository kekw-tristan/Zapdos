#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "meshGenerator.h"

using namespace DirectX;
using namespace Microsoft::WRL;

struct sVertex;
struct sMeshGeometry;
struct sFrameResource; 
struct sRenderItem; 
struct sPassConstants;

class cWindow;
class cTimer;

class cSwapChainManager;
class cDeviceManager;
class cBufferManager;
class cPipelineManager;

template<typename T>
class cUploadBuffer;

constexpr int c_NumberOfFrameResources = 3; 

class cDirectX12
{
	public:

		cDirectX12()	= default;
		~cDirectX12()	= default;

		cDirectX12(const cDirectX12&) = delete;
		cDirectX12& operator=(const cDirectX12&) = delete;

	public:

		void Initialize(cWindow* _pWindow, cTimer* _pTimer);
		void Finalize();
		
	public:
		void InitializeVertices();

		void InitializeMesh(cMeshGenerator::sMeshData& _rMeshData, std::vector<sVertex>& _rVertecis, std::vector<std::uint16_t>& _rIndices, std::string& _rName, XMFLOAT4 _rColor);
		void InitializeGeometryBuffer(std::vector<sVertex>& _rVertecis, std::vector<std::uint16_t>& _rIndices); 

		void Update(XMMATRIX _view);
		void Draw(); 
		float GetAspectRatio() const;
		void CalculateFrameStats() const;
		void OnResize();

	private:

		void UpdateObjectCB();
		void UpdatePassCB();

	private:

		void InitializeFrameResources();
		void WaitForCurrentFrameResourceIfInUse(); 
		void WaitForGPU();

	private:

		cWindow* m_pWindow;
		cTimer*	 m_pTimer;

	private:

		sMeshGeometry* m_pGeometry;
		
		XMFLOAT4X4 m_proj;
		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;

		cSwapChainManager* m_pSwapChainManager;
		cDeviceManager* m_pDeviceManager;
		cBufferManager* m_pBufferManager;
		cPipelineManager* m_pPipelineManager;

		std::vector<sFrameResource*>	m_frameResources;
		std::vector<sRenderItem*>		m_renderItems;
		sFrameResource*	m_pCurrentFrameResource;
		int m_currentFrameResourceIndex; 

		std::unordered_map<std::string, sMeshGeometry*> m_geometries; 
};
