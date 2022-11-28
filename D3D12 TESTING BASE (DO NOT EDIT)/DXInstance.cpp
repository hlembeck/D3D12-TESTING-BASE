#include "DXInstance.h"

DXInstance::DXInstance(UINT width, UINT height) : m_width(width), m_height(height) {
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXInstance::~DXInstance() {

}

void DXInstance::GetAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter) {
	UINT i = 0;
	ComPtr<IDXGIAdapter1> adapter;
	while (pFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		//Don't use the basic render driver software adapter.
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}
		*ppAdapter = adapter.Detach();
		return;
	}
	*ppAdapter = NULL;
}

float DXInstance::GetAspectRatio() {
	return m_aspectRatio;
}