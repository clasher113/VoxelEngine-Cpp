#ifndef ADAPTER_READER_HPP
#define ADAPTER_READER_HPP

#include <vector> 
#include <string>
#include <d3d11_1.h>
#include <wrl/client.h>

class AdapterData {
public:
	AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter);
	Microsoft::WRL::ComPtr<IDXGIAdapter> m_adapter;
	DXGI_ADAPTER_DESC m_description;
	std::string m_vendor;
};

class AdapterReader {
public:
	static const std::vector<AdapterData>& GetAdapters();
private:
	static inline std::vector<AdapterData> s_m_adapters;
};

#endif // !ADAPTER_READER_HPPw