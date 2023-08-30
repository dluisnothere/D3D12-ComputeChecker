#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <stdint.h>

#include "vendor/stdafx.h"

#include "ComputeChecker.h"
// #include "Helpers.h"

#define FRAMES 2
#define WIDTH 1200
#define HEIGHT 800
#define NUM_SQUARES_X 2
#define NUM_SQUARES_Y 2

using namespace Microsoft::WRL;

// Main message handler for the sample.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ComputeChecker* checkerSample = reinterpret_cast<ComputeChecker*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
	{
		// Save the DXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;

	case WM_KEYDOWN:
		if (checkerSample)
		{
			// checkerSample->OnKeyDown(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_KEYUP:
		if (checkerSample)
		{
			// checkerSample->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;

	case WM_PAINT:
		if (checkerSample)
		{
			checkerSample->OnUpdate();
			checkerSample->OnRender();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	ComputeChecker checkerSample = ComputeChecker(NUM_SQUARES_X, NUM_SQUARES_Y, WIDTH, HEIGHT);

	// Create a window
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"ComputeChecker";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(WIDTH), static_cast<LONG>(HEIGHT)};
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	checkerSample.m_hwnd = CreateWindowEx(NULL,
		windowClass.lpszClassName,
		L"Compute Checker Window",
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		WIDTH,
		HEIGHT,
		nullptr,
		nullptr,
		hInstance,
		&checkerSample);

	checkerSample.OnInit();

	ShowWindow(checkerSample.m_hwnd, nCmdShow);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	checkerSample.OnDestroy();

	return 0;

}