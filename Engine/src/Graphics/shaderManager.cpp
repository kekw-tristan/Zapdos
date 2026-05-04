#include "shaderManager.h"

#include "directx12Util.h"

// --------------------------------------------------------------------------------------------------------------------------

cShaderManager::cShaderManager()
    : m_shaders()
{
}

// --------------------------------------------------------------------------------------------------------------------------

cShaderManager::~cShaderManager()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cShaderManager::Load(const std::string& _rName, const std::wstring& _rFile, const std::string& _rEntry, const std::string& _rTarget)
{
    m_shaders[_rName] = cDirectX12Util::CompileShader(_rFile, nullptr, _rEntry, _rTarget);
}

// --------------------------------------------------------------------------------------------------------------------------

ID3DBlob* cShaderManager::GetShader(const std::string& _rName) const
{
    return m_shaders.at(_rName).Get();
}

// --------------------------------------------------------------------------------------------------------------------------