#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include "graphics/material.h"
#include "graphics/meshGenerator.h"

using namespace DirectX;
using namespace Microsoft::WRL;

struct sVertex;
struct sMeshGeometry;
struct sFrameResource;
struct sLightConstants;
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

constexpr int c_NumberOfFrameResources = 2; 

class cDirectX12
{
	public:

		cDirectX12()	= default;
		~cDirectX12()	= default;

		cDirectX12(const cDirectX12&) = delete;
		cDirectX12& operator=(const cDirectX12&) = delete;

	public:

		void Initialize(cWindow* _pWindow, cTimer* _pTimer, unsigned int _maxNumberOfRenderItems, unsigned int _maxNumberOfLights);
		void Finalize();
		
	public:

		void InitializeMesh(cMeshGenerator::sMeshData& _rMeshData, std::string& _rName, XMFLOAT4 _rColor);
		sMeshGeometry* InitializeGeometryBuffer(); 

		void Update(XMMATRIX _view,  XMFLOAT3 _eyePos, std::vector<sRenderItem>* _renderItems, std::vector<sLightConstants>* _pLights);
		void Draw(); 
		float GetAspectRatio() const;
		void CalculateFrameStats() const;
		void OnResize();

		sMeshGeometry* GetGeometry(); 

	private:

		void UpdateObjectCB();
		void UpdatePassCB();
		void UpdateLightCB();

	private:

		void InitializeFrameResources();
		void WaitForCurrentFrameResourceIfInUse(); 
		void WaitForGPU();

	private:

		cWindow* m_pWindow;
		cTimer*	 m_pTimer;

	private:

		unsigned int m_maxNumberOfRenderItems; 
		unsigned int m_maxNumberOfLights; 
		sMeshGeometry* m_pGeometry;
		
		XMFLOAT4X4 m_proj;
		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;

		XMFLOAT3 m_eyePos;

		cSwapChainManager*	m_pSwapChainManager;
		cDeviceManager*		m_pDeviceManager;
		cBufferManager*		m_pBufferManager;
		cPipelineManager*	m_pPipelineManager;

		std::vector<sFrameResource*>	m_frameResources;
		std::vector<sRenderItem>*		m_pRenderItems;
		std::vector<sLightConstants>*	m_pLights;

		sFrameResource*	m_pCurrentFrameResource;
		int m_currentFrameResourceIndex; 

		std::unordered_map<std::string, sMeshGeometry*> m_geometries; 

		std::vector<sVertex>		m_vertecis; 
		std::vector<std::uint16_t>	m_indices;
};
