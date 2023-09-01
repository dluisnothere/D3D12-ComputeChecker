#pragma once
#include "d3dx12.h"

#include <Windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

#include <stdint.h>

// DEBUG
#include "Helpers.h"

using Microsoft::WRL::ComPtr;

struct Vertex {
	float x, y; // 2d Position
	float u, v; // texture coordinates
};

/// <summary>
/// This class generates a checkerboard pattern with input X and Y values
/// using a compute shader to generate a texture, then rendering that texture.
/// </summary>
class ComputeChecker {
public: 
	// Windows variable
	HWND m_hwnd;
private:
	// Variables for number of squares horizontally
	int m_numSquaresX;

	// Variables for number of squares vertically
	int m_numSquaresY;

	// Variables for window width
	UINT m_windowWidth;

	// Variables for window height
	UINT m_windowHeight;

	// Asset path information
	std::wstring m_assetsPath;

	// texture that the checker result is written to
	ComPtr<ID3D12Resource> m_checkerTexture;

	// Type of buffer 
	static const UINT64 Framecount = 2;

	// TODO: DUMB DEBUG FLAG TO KEEP TRACK OF WHETHER THE COMPUTE SHADER HAS ALREADY RUN
	bool m_textureTransitioned = false;


	// Pipeline objects
	ComPtr<ID3D12Device> m_device;

	ComPtr<IDXGISwapChain4> m_swapChain;

	// used in the compute pipeline
	ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
	ComPtr<ID3D12RootSignature> m_computeRootSignature;
	ComPtr<ID3D12PipelineState> m_computePipelineState;
	UINT m_srvDescriptorSize;
	ComPtr<ID3D12CommandAllocator> m_computeCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;

	// CPU and GPU handles
	D3D12_CPU_DESCRIPTOR_HANDLE m_samplerHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandleCpu;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandleCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE m_uavHandleGpu;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandleGpu;
	UINT64 m_rowPitch = (m_windowWidth * 4 + 255)& ~255;

	// Texture resource barrier
	D3D12_RESOURCE_BARRIER m_textureBarrier;

	// used in the graphics pipeline
	ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12Resource> m_renderTargets[Framecount];
	ComPtr<ID3D12RootSignature> m_graphicsRootSignature;
	ComPtr<ID3D12PipelineState> m_graphicsPipelineState;
	UINT m_rtvDescriptorSize;
	ComPtr<ID3D12CommandAllocator> m_graphicsCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_graphicsCommandList;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	// App resources
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;


	// synchronization objects
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

private:
	std::wstring GetAssetFullPath(LPCWSTR assetName);


public: 
	/// <summary>
	/// Constructor
	/// </summary>
	ComputeChecker(int numSquareX, int numSquaresY, UINT windowWidth, UINT windowHeight);

	void OnInit();

	void OnUpdate();

	void OnRender();

	void OnDestroy();

	bool LoadPipeline();

	void LoadAssets();

	void PopulateGraphicsCommandList();

	void PopulateComputeCommandList();

	void WaitForPreviousFrame();

};