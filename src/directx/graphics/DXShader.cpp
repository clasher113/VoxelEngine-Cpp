#ifdef USE_DIRECTX
#include "DXShader.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"
#include "../util/InputLayoutBuilder.hpp"
#include "../../util/stringutil.h"

#include <d3dcompiler.h>

Shader::Shader(ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout) :
	m_p_vertexShader(vertexShader),
	m_p_pixelShader(pixelShader),
	m_p_inputLayout(inputLayout)
{
}

Shader::~Shader() {
	m_p_vertexShader->Release();
	m_p_pixelShader->Release();
}

void Shader::use() const {
	auto context = DXDevice::getContext();
	context->VSSetShader(m_p_vertexShader, 0, 0);
	context->PSSetShader(m_p_pixelShader, 0, 0);
	context->IASetInputLayout(m_p_inputLayout);
}

Shader* Shader::loadShader(const std::wstring_view& shaderFile) {
	ID3D10Blob* VS, * PS;
	ID3D10Blob* errorMsg = nullptr;
	HRESULT errorCode = S_OK;
	UINT flag1 = 0, flag2 = 0;
#ifdef _DEBUG
	flag1 = D3DCOMPILE_DEBUG;
	flag2 = D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG
	errorCode = D3DCompileFromFile(shaderFile.data(), 0, 0, "VShader", "vs_4_0", flag1, flag2, &VS, &errorMsg);

	CHECK_ERROR2(errorCode, L"Failed to compile vertex shader:\n" +
		util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
		errorMsg = nullptr;
	}
	errorCode = D3DCompileFromFile(shaderFile.data(), 0, 0, "PShader", "ps_4_0", flag1, flag2, &PS, &errorMsg);
	
	CHECK_ERROR2(errorCode, L"Failed to compile pixel shader:\n" +
		util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));

	if (errorMsg != nullptr) {
		DXError::throwWarn(util::str2wstr_utf8((char*)errorMsg->GetBufferPointer()));
		errorMsg->Release();
	}

	auto device = DXDevice::getDevice();

	ID3D11VertexShader* pVS;
	ID3D11PixelShader* pPS;

	CHECK_ERROR2(device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS),
		L"Failed to create vertex shader:\n" + std::wstring(shaderFile));
	CHECK_ERROR2(device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS),
		L"Failed to create pixel shader:\n" + std::wstring(shaderFile));

	auto context = DXDevice::getContext();

	static int index = 0;
	switch (index) {
	case 0: InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("TEXCOORD", 2);
			InputLayoutBuilder::add("LIGHT", 1);
			break; // main
	case 1:	InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("COLOR", 4);
			break; // lines
	case 2: InputLayoutBuilder::add("POSITION", 2);
			InputLayoutBuilder::add("UV", 2);
			InputLayoutBuilder::add("COLOR", 4);
			break; // ui
	case 3:	InputLayoutBuilder::add("POSITION", 3);
			InputLayoutBuilder::add("TEXCOORD", 2);
			InputLayoutBuilder::add("COLOR", 4);
			break; // ui3d
	case 4: InputLayoutBuilder::add("POSITION", 2);
			break; // skybox background
	case 5: InputLayoutBuilder::add("POSITION", 2);
			break; // skybox generator
	}

	ID3D11InputLayout* pLayout;
	CHECK_ERROR2(device->CreateInputLayout(InputLayoutBuilder::get(), InputLayoutBuilder::getSize(), VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout),
		L"Failed to create input layoyt for shader:\n" + std::wstring(shaderFile));

	InputLayoutBuilder::clear();
	index++;

	VS->Release();
	PS->Release();

	return new Shader(pVS, pPS, pLayout);
}

#endif // USE_DIRECTX