#pragma once
#include "Shared.h"

class DXInstance {
public:
	DXInstance(UINT width, UINT height);
	virtual ~DXInstance();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;
protected:
	void GetAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
	float GetAspectRatio();
	UINT m_width;
	UINT m_height;
private:
	float m_aspectRatio;
};