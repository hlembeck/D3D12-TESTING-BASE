#include "Game.h"
#include "Application.h"

Game::Game(UINT width, UINT height) : DXInstance(width, height), m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)), m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)) {}

Game::~Game() {}

void Game::OnInit() {
    LoadGraphicsPipeline();
    LoadAssets();
}

void Game::LoadGraphicsPipeline() {
    UINT dxgiFactoryFlags = 0;
    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(factory.Get(), &adapter);

    ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_12_0,
        IID_PPV_ARGS(&m_device)
    ));

    //Create Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT , D3D12_COMMAND_QUEUE_FLAG_NONE };

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        m_width,
        m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        FALSE,
        {1,0}, //sample desc
        DXGI_USAGE_RENDER_TARGET_OUTPUT,
        NUMFRAMES,
        DXGI_SCALING_STRETCH, //Scaling stretch
        DXGI_SWAP_EFFECT_FLIP_DISCARD,

    };

    ComPtr<IDXGISwapChain1> swapChain;
    //IDXGIFactory2 necessary for CreateSwapChainForHwnd
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        Application::GetHWND(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    //DXGI won't respond to alt-enter - WndProc is responsible.
    ThrowIfFailed(factory->MakeWindowAssociation(Application::GetHWND(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain)); //Original swap chain interface now represented by m_swapChain
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        NUMFRAMES,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        0 //Single Adapter
    };
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    // Create a RTV for each frame.
    for (UINT i = 0; i < NUMFRAMES; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        m_renderTargets[i]->SetName(L"Render Target");
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void Game::LoadAssets() {

    CreateRootSignature();

    //Create Pipeline State Objects
    CreatePipelineState();

    //Create Command List
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }

    WaitForPreviousFrame();
}

void Game::CreateRootSignature() {
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    CD3DX12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
    rootParameters[0].InitAsDescriptorTable(1, descriptorRange); //SRV for the texture that was rendered to.
    rootParameters[1].InitAsConstantBufferView(0);

    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

    CD3DX12_STATIC_SAMPLER_DESC terrainSampler = CD3DX12_STATIC_SAMPLER_DESC(
        0,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );
    terrainSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;

    CD3DX12_STATIC_SAMPLER_DESC texSampler = CD3DX12_STATIC_SAMPLER_DESC(
        1,
        D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP
    );
    texSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    staticSamplers[0] = terrainSampler;
    staticSamplers[1] = texSampler;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 2, staticSamplers, rootSignatureFlags);
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob));
    ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void Game::OnUpdate() {}

void Game::OnRender() {
    //Since WaitForPreviousFrame is called both before the first render and after each render, calling (Command Allocator) Reset in FillCommandList is safe.
    FillCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void Game::OnDestroy() {
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void Game::WaitForPreviousFrame() {
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;
    if (m_fence->GetCompletedValue() < fence) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Game::FillCommandList() {
    ThrowIfFailed(m_commandAllocator->Reset());

    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    //Render to back buffer
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &resourceBarrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);


    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Indicate that the back buffer will now be used to present.
    resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &resourceBarrier);

    ThrowIfFailed(m_commandList->Close());
}

void Game::CreatePipelineState() {

    //Compile and load shaders
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"BaseVS.hlsl", NULL, NULL, "main", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    printf("Compiled main vertex shader\n");

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateObjectDesc = {};
    pipelineStateObjectDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    pipelineStateObjectDesc.pRootSignature = m_rootSignature.Get();
    pipelineStateObjectDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    pipelineStateObjectDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pipelineStateObjectDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineStateObjectDesc.SampleMask = UINT_MAX;
    pipelineStateObjectDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateObjectDesc.NumRenderTargets = 1;
    pipelineStateObjectDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateObjectDesc.SampleDesc.Count = 1;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(&m_pipelineState)));
}