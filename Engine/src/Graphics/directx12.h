#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

int constexpr c_swapChainBufferCount = 2;

struct sVertex;
struct sMeshGeometry;

class cWindow;
class cTimer;
	
template<typename T>
class cUploadBuffer;

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
		void InitializeShader();
		void Update(XMMATRIX _view);
		void Draw(); 
		float GetAspectRatio() const;
		void CalculateFrameStats() const;
		void OnResize();

	public:

		ComPtr<ID3D12Device> GetDevice() const;
		ComPtr<ID3D12GraphicsCommandList> GetCommandList() const;

	private:

		void InitializeDeviceAndFactory();
		void InitializeFenceAndDescriptor();
		void Check4XMSAAQualitySupport();
		void InitializeCommandQueueAndList();
		void InitializeSwapChain();
		void InitializeDescriptorHeaps();
		void InitializeRenderTargetView();
		void InitializeDepthStencilView(); 
		void InitializeViewPort();
		void InitializeRootSignature();
		void InitializeRasterierState();
		void InitializePipelineStateObject();
		void InitializeConstantBuffer();
	
	private:

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackbufferView() const; 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const; 

		ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

		void FlushCommandQueue();


	private:

		cWindow* m_pWindow;
		cTimer*	 m_pTimer;

	private:

		ComPtr<IDXGIFactory7>	m_pDxgiFactory;
		ComPtr<IDXGISwapChain4> m_pSwapChain;

		ComPtr<ID3D12Device>	m_pDevice;
		ComPtr<ID3D12Fence>		m_pFence;

		ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
		ComPtr<ID3D12CommandAllocator>		m_pDirectCmdListAlloc;
		ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
		
		ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_pDsvHeap; 

		D3D12_VIEWPORT m_viewPort;

		ComPtr<ID3D12Resource> m_pSwapChainBuffer[c_swapChainBufferCount];
		ComPtr<ID3D12Resource> m_pDepthStencilBuffer;

		DXGI_FORMAT m_backBufferFormat; 
		DXGI_FORMAT m_depthStencilFormat; 

		int m_4xMsaaQuality; 

		int m_rtvDescriptorSize;
		int m_dsvDescriptorSize;
		int m_cbvSrvDescriptorSize;

		int m_currentBackBuffer;

		UINT m_currentFence;

	private:

		struct sObjectConstants
		{
			DirectX::XMFLOAT4X4 worldViewProj;
			sObjectConstants() { DirectX::XMStoreFloat4x4(&worldViewProj, DirectX::XMMatrixIdentity()); }
		};


		UINT m_elementByteSize;
		UINT m_numElements;

		ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;

		cUploadBuffer<sObjectConstants>* m_pObjectCB;
		sMeshGeometry* m_pBoxGeometry;

		ComPtr<ID3D12RootSignature> m_pRootSignature;
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
		
		ComPtr<ID3DBlob> m_pVsByteCode;
		ComPtr<ID3DBlob> m_pPsByteCode;

		ComPtr<ID3D12PipelineState> m_pPso;
	
		XMFLOAT4X4 m_proj;
		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;


};
