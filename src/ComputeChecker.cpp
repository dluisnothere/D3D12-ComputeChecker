// D3D12-ComputeChecker.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "ComputeChecker.h"
#include <iostream>

// TODO: initialize the uninitiazlied valeus
ComputeChecker::ComputeChecker(int numSquaresX, int numSquaresY, UINT windowWidth, UINT windowHeight) :
	m_numSquaresX(numSquaresX), m_numSquaresY(numSquaresY),
	m_windowWidth(windowWidth), m_windowHeight(windowHeight),
	m_viewport(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight)),
	m_scissorRect(0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight)),
	m_rtvDescriptorSize(0)
{
	
}

std::wstring ComputeChecker::GetAssetFullPath(LPCWSTR assetName) {
	if (assetName == nullptr) {
		throw std::invalid_argument("Asset name must not be null.");
	}

	return m_assetsPath + assetName;
}

void ComputeChecker::OnInit() {
	LoadPipeline();
	LoadAssets();

	// Populate the compute command list only once at the beginning
	PopulateComputeCommandList();
	// Execute the compute command list only once at the beginning
	ID3D12CommandList* ppCommandLists[] = { m_computeCommandList.Get() };
	m_computeCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_checkerTexture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;  // The state used for compute shader write
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	// TODO: this might have to be the compute command list.... not sure
	m_graphicsCommandList->ResourceBarrier(1, &barrier);
}

bool ComputeChecker::LoadPipeline() {
	UINT64 dxgiFactoryFlags = 0;

	// Enable Direct X debug layer
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}

	// Create factory
	ComPtr<IDXGIFactory6> ptrFactory6;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&ptrFactory6));

	// Get Adapter
	ComPtr<IDXGIAdapter1> ptrAdapter1;
	HRESULT hr = ptrFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&ptrAdapter1));
	switch (hr) {
		case S_OK:
			break;
		case DXGI_ERROR_NOT_FOUND:
			return false;
			break;
		default:
			return false;
			break;
	}

	// Create Device
	// Create a temp device to get the highest feature level
	ComPtr<ID3D12Device> ptrTempDevice;
	D3D12CreateDevice(ptrAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ptrTempDevice));

	D3D_FEATURE_LEVEL reqArray[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS flds;
	// Zero memory
	ZeroMemory(&flds, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));

	// Fill descriptor for device
	flds.NumFeatureLevels = _countof(reqArray);
	flds.pFeatureLevelsRequested = reqArray;

	// Execute feature request
	ptrTempDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &flds, sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));

	// Create the real device on the highest level. The temp device will be deleted after this function is over.
	D3D12CreateDevice(ptrAdapter1.Get(), flds.MaxSupportedFeatureLevel, IID_PPV_ARGS(&m_device));

	// Describe and create the compute queue
	D3D12_COMMAND_QUEUE_DESC cQueueDesc = {};
	cQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	// Describe and create the graphics queue
	D3D12_COMMAND_QUEUE_DESC gQueueDesc = {};
	gQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	gQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	m_device->CreateCommandQueue(&gQueueDesc, IID_PPV_ARGS(&m_graphicsCommandQueue));
	m_device->CreateCommandQueue(&cQueueDesc, IID_PPV_ARGS(&m_computeCommandQueue));

	// Describe and create the swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = Framecount;
	swapChainDesc.Width = m_windowWidth;
	swapChainDesc.Height = m_windowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ptrFactory6->CreateSwapChainForHwnd(
		m_graphicsCommandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it
		m_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	swapChain.As(&m_swapChain);
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create render target view descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = Framecount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create shader resource view descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return true;
}

void ComputeChecker::LoadAssets() {
	// Create an empty root signature for graphics pipeline state object
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_graphicsRootSignature));
	}

	// Create an empty root signature for compute pipeline state object
{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature));
	}

	// Create the texture that the compute shader will write its results to
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = m_windowWidth;
		textureDesc.Height = m_windowHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		m_device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_checkerTexture)
		);

		m_checkerTexture->SetName(L"Texture Result");

		// Create SRV for texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		m_device->CreateShaderResourceView(m_checkerTexture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create UAV for texture
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		m_device->CreateUnorderedAccessView(m_checkerTexture.Get(), nullptr, &uavDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
	}
	
	// Compile the shaders
	ComPtr<ID3DBlob> computeShader;
	ComPtr<ID3DBlob> pixelShader;
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> error;
#if defined (_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	HRESULT hrc = D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &error);
	// DEBUG
	if (FAILED(hrc)) {
		OutputDebugStringA((char*)error->GetBufferPointer());
	}

	HRESULT hrv = D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error);
	// DEBUG
	if (FAILED(hrv)) {
		OutputDebugStringA((char*)error->GetBufferPointer());
	}
	D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);


	// Define vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD0",    0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	
	// Create the compute pipeline state
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = m_computeRootSignature.Get();
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	m_device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_computePipelineState));

	// Create the graphics pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPsoDesc = {};
	graphicsPsoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	graphicsPsoDesc.pRootSignature = m_graphicsRootSignature.Get();
	graphicsPsoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	graphicsPsoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	graphicsPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	graphicsPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	graphicsPsoDesc.DepthStencilState.DepthEnable = FALSE;
	graphicsPsoDesc.DepthStencilState.StencilEnable = FALSE;
	graphicsPsoDesc.SampleMask = UINT_MAX;
	graphicsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPsoDesc.NumRenderTargets = 1;
	graphicsPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	graphicsPsoDesc.SampleDesc.Count = 1;
	
	m_device->CreateGraphicsPipelineState(&graphicsPsoDesc, IID_PPV_ARGS(&m_graphicsPipelineState));

	// Create the command lists
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocator.Get(), m_computePipelineState.Get(), IID_PPV_ARGS(&m_computeCommandList));
	m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCommandAllocator.Get(), m_graphicsPipelineState.Get(), IID_PPV_ARGS(&m_graphicsCommandList));

	// Close the command lists as their default starting states
	m_computeCommandList->Close();
	m_graphicsCommandList->Close();

	// Create the vertexbuffer, which should just be a quad that fits the screen
	Vertex screenQuadVertices[] = {
		{-1.0f, -1.0f, 0.0f, 1.0f},
		{-1.0f,  1.0f, 0.0f, 0.0f},
		{ 1.0f,  1.0f, 1.0f, 0.0f},
		{ 1.0f, -1.0f, 1.0f, 1.0f},
	};

	// Indices for the quad
	uint16_t screenQuadIndices[] = {
		0, 1, 2,
		0, 2, 3
	};

	const UINT64 vertexBufferSize = sizeof(screenQuadVertices);

	// TODO: Optimize this by using default heaps instead of upload heaps
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));

	// Copy the vertex data to the vertex buffer
	UINT8* vertexDataBegin;
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
	memcpy(vertexDataBegin, screenQuadVertices, sizeof(screenQuadVertices));
	m_vertexBuffer->Unmap(0, nullptr);

	// Initialize vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// Create synchronization objects adn wait until assets have been uploaded to the GPU
	{
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			HRESULT_FROM_WIN32(GetLastError());
		}

		// Wait for the command list to execute
		WaitForPreviousFrame();
	}
}

void ComputeChecker::OnUpdate() {

}

void ComputeChecker::OnRender() {
	// Record all the commands we need to render the scene into the command list
	PopulateGraphicsCommandList();

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { m_graphicsCommandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame
	m_swapChain->Present(1, 0);

	WaitForPreviousFrame();
}

void ComputeChecker::OnDestroy() {
	// Wait for the GPU to be done with all resources
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void ComputeChecker::PopulateGraphicsCommandList() {
	// Reset the comamnd allocator. 
	// Should only be reset when the associated command lists have finished execution on the GPU
	m_graphicsCommandAllocator->Reset();

	// When ExecuteCommandList() is called on any command list, the command list can then be reset at any time.
	// They must be reset before re-recording
	m_graphicsCommandList->Reset(m_graphicsCommandAllocator.Get(), m_computePipelineState.Get());

	// Set neccessary state
	m_graphicsCommandList->SetGraphicsRootSignature(m_graphicsRootSignature.Get());

	// TODO: might have to move this. This line binds the completed texture from the compute shader to the pixel shader
	m_graphicsCommandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

	// TODO: What does this function do?
	m_graphicsCommandList->RSSetViewports(1, &m_viewport);
	m_graphicsCommandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_graphicsCommandList->ResourceBarrier(1, &transition);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_graphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record graphics commands
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_graphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_graphicsCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_graphicsCommandList->DrawInstanced(6, 1, 0, 0);

	// Back buffer will now present
	CD3DX12_RESOURCE_BARRIER transition2 = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_graphicsCommandList->ResourceBarrier(1, &transition2);

	// Close the graphics command list
	m_graphicsCommandList->Close();

}

void ComputeChecker::PopulateComputeCommandList() {
	// Reset the command allocator
	m_computeCommandAllocator->Reset();

	// Reset the command list
	m_computeCommandList->Reset(m_computeCommandAllocator.Get(), m_computePipelineState.Get());

	// Set the neccessary state
	m_computeCommandList->SetComputeRootSignature(m_computeRootSignature.Get());

	// Execute the compute shader
	m_computeCommandList->SetPipelineState(m_computePipelineState.Get());
	m_computeCommandList->Dispatch(1, 1, 1);

	// Close the command list
	m_computeCommandList->Close();
}

void ComputeChecker::WaitForPreviousFrame() {
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// TODO: optimize
	// Signal and increment the fence value
	const UINT64 fence = m_fenceValue;
	m_graphicsCommandQueue->Signal(m_fence.Get(), fence);
	m_fenceValue++;

	// Wait until the previous frame is finished
	if (m_fence->GetCompletedValue() < fence) {
		m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}