#pragma once
#include "Shared.h"
#include "DXInstance.h"
/*
This class contains the window procedures.
*/
class Application {
public:
	static int Run(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height, DXInstance* pInstance);

	static HWND GetHWND();
protected:

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static HWND m_hWnd;
};