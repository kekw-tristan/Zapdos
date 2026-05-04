#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <string>

using Microsoft::WRL::ComPtr;

class cShaderManager
{
	public:

		cShaderManager();
		~cShaderManager();

	public:

		void Load(const std::string& _rName,
			const std::wstring& _rFile,
			const std::string&	_rEntry,
			const std::string&	_rTarget);

		ID3DBlob* GetShader(const std::string& _rName) const;

	private:

		std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders;
};