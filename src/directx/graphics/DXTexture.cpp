#ifdef USE_DIRECTX
#include "DXTexture.hpp"
#include "directx/window/DXDevice.hpp"
#include "directx/util/DXError.hpp"
#include "graphics/core/ImageData.hpp"
#include "directx/util/DebugUtil.hpp"
#include "directx/util/TextureUtil.hpp"

uint Texture::MAX_RESOLUTION = 16384;
constexpr UINT MAX_MIP_LEVEL = 3U;

static UINT GetNumMipLevels(UINT width, UINT height) {
	UINT numLevels = 1;
	while (width > 32 && height > 32) {
		if (++numLevels >= MAX_MIP_LEVEL) return MAX_MIP_LEVEL;
		width = std::max(width / 2, 1U);
		height = std::max(height / 2, 1U);
	}
	return numLevels;
}

Texture::Texture(ID3D11Texture2D* texture) :
	m_p_texture(texture),
	m_description()
{
	ID3D11Device* const device = DXDevice::getDevice();
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, nullptr, &m_p_resourceView),
		L"Failed to create shader resource view");

	m_p_texture->GetDesc(&m_description);

	if (m_description.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) {
		ID3D11DeviceContext* const context = DXDevice::getContext();
		context->GenerateMips(m_p_resourceView);
	}
	SET_DEBUG_OBJECT_NAME(m_p_texture, "Texture 2D");
	SET_DEBUG_OBJECT_NAME(m_p_resourceView, "Resource View");
}

Texture::Texture(ubyte* data, uint width, uint height, ImageFormat format) :
	m_description({
	    /* UINT Width */					width,
		/* UINT Height */					height,
		/* UINT MipLevels */				0U,
		/* UINT ArraySize */				1U,
		/* DXGI_FORMAT Format */			DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc */	{
			/* UINT Count */	1,
			/* UINT Quality */	0
		},
		/* D3D11_USAGE Usage */				D3D11_USAGE_DEFAULT,
		/* UINT BindFlags */				D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		/* UINT CPUAccessFlags */			0U,
		/* UINT MiscFlags */				D3D11_RESOURCE_MISC_GENERATE_MIPS
	})
{
	ID3D11Device* const device = DXDevice::getDevice();
	CHECK_ERROR2(device->CreateTexture2D(&m_description, nullptr, &m_p_texture),
		L"Failed to create texture");

	m_srvDescription = {
		/* DXGI_FORMAT Format */				m_description.Format,
		/* D3D11_SRV_DIMENSION ViewDimension */	D3D11_SRV_DIMENSION_TEXTURE2D,
		/* D3D11_TEX2D_SRV Texture2D */			{
			/* UINT MostDetailedMip */	0,
			/* UINT MipLevels */		GetNumMipLevels(width, height)
		}
	};

	CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &m_srvDescription, &m_p_resourceView),
		L"Failed to create shader resource view");

	ID3D11DeviceContext* const context = DXDevice::getContext();

	reload(data);

	context->GenerateMips(m_p_resourceView);

	SET_DEBUG_OBJECT_NAME(m_p_texture, "Texture 2D");
	SET_DEBUG_OBJECT_NAME(m_p_resourceView, "Resource View");
}

Texture::~Texture() {
	m_p_texture->Release();
	m_p_resourceView->Release();
}

void Texture::setNearestFilter() {
	// todo: implement
}

void Texture::bind(unsigned int shaderType, UINT startSlot) const {
	ID3D11DeviceContext* const context = DXDevice::getContext();
	if (shaderType & VERTEX)	context->VSSetShaderResources(startSlot, 1u, &m_p_resourceView);
	if (shaderType & PIXEL)		context->PSSetShaderResources(startSlot, 1u, &m_p_resourceView);
	if (shaderType & GEOMETRY)	context->GSSetShaderResources(startSlot, 1u, &m_p_resourceView);
}

void Texture::reload(ubyte* data) {
	ID3D11DeviceContext* const context = DXDevice::getContext();
	context->UpdateSubresource(m_p_texture, 0u, nullptr, data, m_description.Width * (m_description.Format == DXGI_FORMAT_R8G8B8A8_UNORM ? 4 : 3), 0u);
}

void Texture::reload(const ImageData& image) {
	m_description.Width = image.getWidth();
	m_description.Height = image.getHeight();
	reload(image.getData());
}

void Texture::setMipMapping(bool flag) {
	std::unique_ptr<ImageData> data = readData(false);

	m_p_texture->Release();
	m_p_resourceView->Release();

	if (flag){
		m_description.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		m_description.BindFlags |= D3D11_BIND_RENDER_TARGET;
		m_srvDescription.Texture2D.MipLevels = GetNumMipLevels(m_description.Width, m_description.Height);
	}
	else {
		m_description.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
		m_description.BindFlags &= ~D3D11_BIND_RENDER_TARGET;
		m_srvDescription.Texture2D.MipLevels = 1;
	}

	ID3D11Device* const device = DXDevice::getDevice();
	CHECK_ERROR2(device->CreateTexture2D(&m_description, nullptr, &m_p_texture),
		L"Failed to create texture");
	CHECK_ERROR2(device->CreateShaderResourceView(m_p_texture, &m_srvDescription, &m_p_resourceView),
		L"Failed to create shader resource view");

	reload(*data);

	if (flag) {
		ID3D11DeviceContext* const context = DXDevice::getContext();
		context->GenerateMips(m_p_resourceView);
	}
}

std::unique_ptr<ImageData> Texture::readData(bool flipY) {
	auto data = std::make_unique<ubyte[]>(m_description.Width * m_description.Height * 4);
	ID3D11Texture2D* staged = nullptr;
	TextureUtil::stageTexture(m_p_texture, &staged);
	TextureUtil::readPixels(staged, data.get(), flipY);
	staged->Release();
	return std::make_unique<ImageData>(
		ImageFormat::rgba8888, m_description.Width, m_description.Height, data.release()
	);
}

ID3D11Texture2D* Texture::getId() const {
	return m_p_texture;
}

ID3D11ShaderResourceView* Texture::getResourceView() const {
	return m_p_resourceView;
}

std::unique_ptr<Texture> Texture::from(const ImageData* image) {
	uint width = image->getWidth();
	uint height = image->getHeight();
	const void* data = image->getData();
	return std::make_unique<Texture>((ubyte*)data, width, height, image->getFormat());
}

#endif // USE_DIRECTX