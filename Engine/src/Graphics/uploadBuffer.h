#include <d3dx12.h>

template<typename T>
class cUploadBuffer
{
public:
    cUploadBuffer(ID3D12Device* _pDevice, UINT _elementCount, bool _isConstantBuffer)
        : m_pMappedData(nullptr)
        , m_pUploadBuffer(nullptr)
        , m_elementByteSize(0)
        , m_isConstantBuffer(_isConstantBuffer)
        , m_elementCount(_elementCount)
    {
        if (m_isConstantBuffer)
        {
            // Align to 256 bytes for constant buffers
            m_elementByteSize = cDirectX12Util::CalculateBufferByteSize(sizeof(T));
        }
        else
        {
            // No alignment for structured buffer
            m_elementByteSize = sizeof(T);
        }

        UINT bufferSize = m_elementByteSize * _elementCount;

        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        // Create the upload buffer resource
        cDirectX12Util::ThrowIfFailed(_pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_pUploadBuffer)
        ));

        // Map the buffer memory
        cDirectX12Util::ThrowIfFailed(m_pUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData)));
    }

    ~cUploadBuffer()
    {
        if (m_pUploadBuffer != nullptr)
            m_pUploadBuffer->Unmap(0, nullptr);
        m_pUploadBuffer = nullptr;
        m_pMappedData = nullptr;
    }

    cUploadBuffer(const cUploadBuffer&) = delete;
    cUploadBuffer& operator=(const cUploadBuffer&) = delete;

    ID3D12Resource* GetResource()
    {
        return m_pUploadBuffer.Get();
    }

    UINT GetElementByteSize()
    {
        return m_elementByteSize;
    }

    UINT GetElementCount()
    {
        return m_elementCount;
    }

    void CopyData(int _elementIndex, const T& _rData)
    {
        // Copy data at proper offset
        memcpy(&m_pMappedData[_elementIndex * m_elementByteSize], &_rData, sizeof(T));
    }

private:
    BYTE* m_pMappedData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadBuffer;
    UINT m_elementByteSize;
    bool m_isConstantBuffer;
    UINT m_elementCount;
};
