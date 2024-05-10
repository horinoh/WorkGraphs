// GenericPrograms.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "GenericPrograms.h"

#define MAX_LOADSTRING 100

#include <array>
#include <algorithm>
#include <format>
#include <filesystem>

#include <d3d12.h>
#include <atlbase.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 613; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = reinterpret_cast<const char*>(u8".\\D3D12\\"); }

#ifdef _DEBUG
#define VERIFY_SUCCEEDED(X) { const auto _HR = (X); if(FAILED(_HR)) { OutputDebugStringA(std::data(std::format("HRESULT = {:#x}\n", static_cast<UINT32>(_HR)))); __debugbreak(); } }
#else
#define VERIFY_SUCCEEDED(X) (X) 
#endif

template<typename T> static constexpr size_t TotalSizeOf(const std::vector<T>& rhs) { return sizeof(T) * std::size(rhs); }
template<typename T, size_t U> static constexpr size_t TotalSizeOf(const std::array<T, U>& rhs) { return sizeof(rhs); }

std::vector<CComPtr<ID3D12GraphicsCommandList10>> GraphicsCommandList10s;
CComPtr<ID3D12CommandQueue> CommandQueue;
CComPtr<ID3D12Fence> Fence;
CComPtr<IDXGISwapChain4> SwapChain4;
std::vector<CComPtr<ID3D12Resource>> SwapChainResources;
CComPtr<ID3D12RootSignature> RootSignature;
CComPtr<ID3D12Resource> VertexBuffer;
CComPtr<ID3D12Resource> IndexBuffer;
CComPtr<ID3D12Resource> IndirectBuffer;

void CreateBuffer(ID3D12Device* Device, const UINT64 Size, const D3D12_RESOURCE_FLAGS Flags, const D3D12_HEAP_TYPE Type, const D3D12_RESOURCE_STATES State, ID3D12Resource** Resource)
{
    const D3D12_RESOURCE_DESC RD = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = Size, .Height = 1, .DepthOrArraySize = 1, .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = DXGI_SAMPLE_DESC({.Count = 1, .Quality = 0 }),
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = Flags,
    };
    const D3D12_HEAP_PROPERTIES HP = {
        .Type = Type,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0,
    };
    VERIFY_SUCCEEDED(Device->CreateCommittedResource(&HP, D3D12_HEAP_FLAG_NONE, &RD, State, nullptr, IID_PPV_ARGS(Resource)));
}
void CreateDefaultBuffer(ID3D12Device* Device, const UINT64 Size, ID3D12Resource** Resource) { CreateBuffer(Device, Size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, Resource); };
void CreateUploadBuffer(ID3D12Device* Device, const UINT64 Size, ID3D12Resource** Resource, const void* Src = nullptr) {
    CreateBuffer(Device, Size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON, Resource);
    if (nullptr != Src) {
        BYTE* Data;
        VERIFY_SUCCEEDED((*Resource)->Map(0, nullptr, reinterpret_cast<void**>(&Data))); {
            memcpy(Data, Src, Size);
        } (*Resource)->Unmap(0, nullptr);
    }
};

void WaitForFence(ID3D12Fence* Fence, ID3D12CommandQueue* CQ)
{
    auto Value = Fence->GetCompletedValue();
    VERIFY_SUCCEEDED(CQ->Signal(Fence, ++Value));
    if (Fence->GetCompletedValue() < Value) {
        auto hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (nullptr != hEvent) [[likely]] {
            VERIFY_SUCCEEDED(Fence->SetEventOnCompletion(Value, hEvent));
            WaitForSingleObject(hEvent, INFINITE);
            CloseHandle(hEvent);
        }
    }
}

void Barrier(ID3D12GraphicsCommandList* GCL, ID3D12Resource* Resource, const D3D12_RESOURCE_STATES Before, const D3D12_RESOURCE_STATES After) 
{
    const std::array RBs = {
      D3D12_RESOURCE_BARRIER({
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition = D3D12_RESOURCE_TRANSITION_BARRIER({
              .pResource = Resource,
              .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
              .StateBefore = Before,
              .StateAfter = After,
          }),
      }),
    };
    GCL->ResourceBarrier(static_cast<UINT>(std::size(RBs)), std::data(RBs));
}


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GENERICPROGRAMS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GENERICPROGRAMS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GENERICPROGRAMS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GENERICPROGRAMS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
    {
#ifdef _DEBUG
        CComPtr<ID3D12Debug> Debug;
        VERIFY_SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug)));
        Debug->EnableDebugLayer();
#endif

        //!< ファクトリー
        CComPtr<IDXGIFactory4> Factory;
#ifdef _DEBUG
        VERIFY_SUCCEEDED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&Factory)));
#else
        VERIFY_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)));
#endif

        //!< デバイス
        CComPtr<ID3D12Device> Device;
        VERIFY_SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device)));
        CComPtr<ID3D12Device14> Device14;
        Device14 = Device;

        //!< コマンドキュー
        constexpr D3D12_COMMAND_QUEUE_DESC CQD = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0
        };
        VERIFY_SUCCEEDED(Device->CreateCommandQueue(&CQD, IID_PPV_ARGS(&CommandQueue)));

        //!< フェンス
        VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

        RECT Rect;
        GetClientRect(hWnd, &Rect);
        const auto W = Rect.right - Rect.left, H = Rect.bottom - Rect.top;

        //!< スワップチェイン
        DXGI_SWAP_CHAIN_DESC SCD = {
            .BufferDesc = DXGI_MODE_DESC({
                .Width = static_cast<UINT>(W), .Height = static_cast<UINT>(H),
                .RefreshRate = DXGI_RATIONAL({.Numerator = 60, .Denominator = 1 }),
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling = DXGI_MODE_SCALING_UNSPECIFIED
            }),
            .SampleDesc = DXGI_SAMPLE_DESC({.Count = 1, .Quality = 0 }),
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .OutputWindow = hWnd,
            .Windowed = TRUE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        };
        CComPtr<IDXGISwapChain> SwapChain;
        VERIFY_SUCCEEDED(Factory->CreateSwapChain(CommandQueue, &SCD, &SwapChain));
        SwapChain4 = SwapChain;

        const auto DHD = D3D12_DESCRIPTOR_HEAP_DESC({
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = SCD.BufferCount,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0 
        });
        CComPtr<ID3D12DescriptorHeap> SwapChainDH;
        VERIFY_SUCCEEDED(Device->CreateDescriptorHeap(&DHD, IID_PPV_ARGS(&SwapChainDH)));
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SwapChainHandles;
        auto CDH = SwapChainDH->GetCPUDescriptorHandleForHeapStart();
        const auto IncSize = Device->GetDescriptorHandleIncrementSize(SwapChainDH->GetDesc().Type);
        for (UINT i = 0; i < SCD.BufferCount; ++i) {
            auto& Res = SwapChainResources.emplace_back();
            VERIFY_SUCCEEDED(SwapChain->GetBuffer(i, IID_PPV_ARGS(&Res)));
            Device->CreateRenderTargetView(Res, nullptr, CDH);

            SwapChainHandles.emplace_back(CDH);
            CDH.ptr += IncSize;
        }

        //!< コマンド
        CComPtr<ID3D12CommandAllocator> CommandAllocator;
        VERIFY_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));
        for (UINT i = 0; i < SCD.BufferCount; ++i) {
            CComPtr<ID3D12CommandList> CL;
            VERIFY_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, nullptr, IID_PPV_ARGS(&CL)));
            GraphicsCommandList10s.emplace_back() = CL;
            VERIFY_SUCCEEDED(GraphicsCommandList10s.back()->Close()); //!< クローズしておく
        }

        //!< ビューポート、シザー
        const std::array Viewports = {
            D3D12_VIEWPORT({.TopLeftX = 0.0f, .TopLeftY = 0.0f, .Width = static_cast<FLOAT>(W), .Height = static_cast<FLOAT>(H), .MinDepth = 0.0f, .MaxDepth = 1.0f}),
        };
        const std::array ScissorRects = {
            D3D12_RECT({.left = 0, .top = 0, .right = W, .bottom = H }),
        };

        //!< ルートシグネチャ
        constexpr std::array<D3D12_ROOT_PARAMETER1, 0> RPs = {};
        constexpr std::array<D3D12_STATIC_SAMPLER_DESC, 0> SSDs = {};
        const D3D12_ROOT_SIGNATURE_DESC1 RSD = {
            .NumParameters = static_cast<UINT>(std::size(RPs)), .pParameters = std::data(RPs),
            .NumStaticSamplers = static_cast<UINT>(std::size(SSDs)), .pStaticSamplers = std::data(SSDs),
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        };
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC VRSD = { .Version = D3D_ROOT_SIGNATURE_VERSION_1_1, .Desc_1_1 = RSD, };
        CComPtr<ID3DBlob> RSBlob, ErrorBlob;
        VERIFY_SUCCEEDED(D3D12SerializeVersionedRootSignature(&VRSD, &RSBlob, &ErrorBlob));
        VERIFY_SUCCEEDED(Device->CreateRootSignature(0, RSBlob->GetBufferPointer(), RSBlob->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

        //!< シェーダ
        CComPtr<ID3DBlob> SB_VS;
        VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "VS.cso").wstring()), &SB_VS));
        const std::array EDs_VS = { D3D12_EXPORT_DESC({.Name = L"VSMain", .ExportToRename = L"*", .Flags = D3D12_EXPORT_FLAG_NONE }) };
        CComPtr<ID3DBlob> SB_PS;
        VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / "PS.cso").wstring()), &SB_PS));
        const std::array EDs_PS = { D3D12_EXPORT_DESC({.Name = L"PSMain", .ExportToRename = L"*", .Flags = D3D12_EXPORT_FLAG_NONE }) };
       
        //!< バーテックスバッファ
        using PosCol = std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>;
        constexpr std::array Vertices = {
            //!< CCW
            PosCol({{ 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } }),
            PosCol({{ -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } }),
            PosCol({{ 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }),
        };
        CreateDefaultBuffer(Device, TotalSizeOf(Vertices), &VertexBuffer);
        const D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {
           .BufferLocation = VertexBuffer->GetGPUVirtualAddress(),
           .SizeInBytes = static_cast<UINT>(TotalSizeOf(Vertices)),
           .StrideInBytes = sizeof(Vertices[0]),
        };
        CComPtr<ID3D12Resource> UploadBuffer_Vertex;
        CreateUploadBuffer(Device, TotalSizeOf(Vertices), &UploadBuffer_Vertex, std::data(Vertices));

        //!< インデックスバッファ
        constexpr std::array Indices = { UINT32(0), UINT32(1), UINT32(2) };
        CreateDefaultBuffer(Device, TotalSizeOf(Indices), &IndexBuffer);
        const D3D12_INDEX_BUFFER_VIEW IndexBufferView = {
            .BufferLocation = IndexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = static_cast<UINT>(TotalSizeOf(Indices)),
            .Format = DXGI_FORMAT_R32_UINT
        };
        CComPtr<ID3D12Resource> UploadBuffer_Index;
        CreateUploadBuffer(Device, TotalSizeOf(Indices), &UploadBuffer_Index, std::data(Indices));

        //!< インダイレクトバッファ
        constexpr D3D12_DRAW_INDEXED_ARGUMENTS DIA = {
            .IndexCountPerInstance = static_cast<UINT32>(std::size(Indices)),
            .InstanceCount = 1,
            .StartIndexLocation = 0,
            .BaseVertexLocation = 0,
            .StartInstanceLocation = 0
        };
        CreateDefaultBuffer(Device, sizeof(DIA), &IndirectBuffer);
        CComPtr<ID3D12Resource> UploadBuffer_Indirect;
        CreateUploadBuffer(Device, sizeof(DIA), &UploadBuffer_Indirect, &DIA);
        constexpr std::array IADs = { 
            D3D12_INDIRECT_ARGUMENT_DESC({.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED }), 
        };
        const D3D12_COMMAND_SIGNATURE_DESC CSD = {
            .ByteStride = sizeof(DIA),
            .NumArgumentDescs = static_cast<UINT>(std::size(IADs)), .pArgumentDescs = std::data(IADs), 
            .NodeMask = 0
        };
        CComPtr<ID3D12CommandSignature> CommandSignature;
        Device->CreateCommandSignature(&CSD, nullptr, IID_PPV_ARGS(&CommandSignature));

        //!< アップロードコマンド発行
        const auto GCL10 = GraphicsCommandList10s[0];
        VERIFY_SUCCEEDED(GCL10->Reset(CommandAllocator, nullptr));
        {
            GCL10->CopyBufferRegion(VertexBuffer, 0, UploadBuffer_Vertex, 0, TotalSizeOf(Vertices));
            Barrier(GCL10, VertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

            GCL10->CopyBufferRegion(IndexBuffer, 0, UploadBuffer_Index, 0, TotalSizeOf(Indices));
            Barrier(GCL10, IndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

            GCL10->CopyBufferRegion(IndirectBuffer, 0, UploadBuffer_Indirect, 0, sizeof(DIA));
            Barrier(GCL10, IndirectBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        }
        VERIFY_SUCCEEDED(GCL10->Close());
        const std::array CLs = { static_cast<ID3D12CommandList*>(GCL10) };
        CommandQueue->ExecuteCommandLists(static_cast<UINT>(std::size(CLs)), std::data(CLs));
        WaitForFence(Fence, CommandQueue);

        //!< ステートオブジェクト
        const D3D12_STATE_OBJECT_CONFIG SOC = { .Flags = D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS };
        const D3D12_GLOBAL_ROOT_SIGNATURE GRS = { .pGlobalRootSignature = RootSignature };
        const D3D12_DXIL_LIBRARY_DESC DLD_VS = {
            .DXILLibrary = D3D12_SHADER_BYTECODE({.pShaderBytecode = SB_VS->GetBufferPointer(), .BytecodeLength = SB_VS->GetBufferSize() }),
            .NumExports = static_cast<UINT>(std::size(EDs_VS)), .pExports = std::data(EDs_VS)
        };
        const D3D12_DXIL_LIBRARY_DESC DLD_PS = {
          .DXILLibrary = D3D12_SHADER_BYTECODE({.pShaderBytecode = SB_PS->GetBufferPointer(), .BytecodeLength = SB_PS->GetBufferSize() }),
          .NumExports = static_cast<UINT>(std::size(EDs_PS)), .pExports = std::data(EDs_PS)
        };

        //!< ジェネリックプログラムへ指定するもの (デフォルト値で良いものは省略可能)
        const std::array IEDs = {
            D3D12_INPUT_ELEMENT_DESC({.SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
            D3D12_INPUT_ELEMENT_DESC({.SemanticName = "COLOR", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0 }),
        };
        const D3D12_INPUT_LAYOUT_DESC ILD = { .pInputElementDescs = std::data(IEDs), .NumElements = static_cast<UINT>(std::size(IEDs)) };
        const D3D12_PRIMITIVE_TOPOLOGY_TYPE PTT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        const D3D12_RT_FORMAT_ARRAY RFA = { .RTFormats = { DXGI_FORMAT_R8G8B8A8_UNORM }, .NumRenderTargets = 1 };
        constexpr D3D12_RASTERIZER_DESC2 RD2 = {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_BACK, .FrontCounterClockwise = TRUE,
            .DepthBias = D3D12_DEFAULT_DEPTH_BIAS, .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP, .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, .DepthClipEnable = TRUE,
            .LineRasterizationMode = D3D12_LINE_RASTERIZATION_MODE_ALIASED,
            .ForcedSampleCount = 0,
            .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
        };
        constexpr D3D12_BLEND_DESC BD = {
            .AlphaToCoverageEnable = TRUE,
            .IndependentBlendEnable = FALSE,
            .RenderTarget = {
                D3D12_RENDER_TARGET_BLEND_DESC({
                    .BlendEnable = FALSE,
                    .LogicOpEnable = FALSE,
                    .SrcBlend = D3D12_BLEND_ONE, .DestBlend = D3D12_BLEND_ZERO, .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_ONE, .DestBlendAlpha = D3D12_BLEND_ZERO, .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .LogicOp = D3D12_LOGIC_OP_NOOP,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                }),
            },
        };
        constexpr D3D12_DEPTH_STENCILOP_DESC1 DSOD = {
            .StencilFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
            .StencilFunc = D3D12_COMPARISON_FUNC_NONE,
            .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        };
        constexpr D3D12_DEPTH_STENCIL_DESC2 DSD = {
            .DepthEnable = FALSE,
            .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc = D3D12_COMPARISON_FUNC_NONE,
            .StencilEnable = FALSE,
            .FrontFace = DSOD,
            .BackFace = DSOD,
            .DepthBoundsTestEnable = FALSE
        };
        constexpr UINT SM = D3D12_DEFAULT_SAMPLE_MASK;
        constexpr DXGI_SAMPLE_DESC SD = { .Count = 1, .Quality = 0 };
        std::array Exports = { L"VSMain", L"PSMain" };
        //!< 非 const にしておき、.NumSubobjects, .ppSubobjects は後でセットする
        D3D12_GENERIC_PROGRAM_DESC GPD = {
           .ProgramName = L"GenericPrograms",
           .NumExports = static_cast<UINT>(std::size(Exports)), .pExports = std::data(Exports),
           //.NumSubobjects = , .ppSubobjects = //!< ここではまだセットしない (できない)
        };

        const std::array SSs = {
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, .pDesc = &SOC }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &GRS }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &DLD_VS }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &DLD_PS }),

            //!< ジェネリックプログラムへ指定するもの
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, .pDesc = &ILD }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, .pDesc = &PTT }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, .pDesc = &RFA }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_RASTERIZER, .pDesc = &RD2 }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_BLEND, .pDesc = &BD }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, .pDesc = &DSD }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, .pDesc = &SM }),
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, .pDesc = &SD }),
            //!< ジェネリックプログラム
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_GENERIC_PROGRAM, .pDesc = &GPD }),
        };
        
        //!< ジェネリックプログラムへ指定するもの
        const std::array SSs_GP = { &SSs[4], &SSs[5], &SSs[6], &SSs[7], &SSs[8], &SSs[9], &SSs[10], &SSs[11] };
        //!< .NumSubobjects, .ppSubobjects はここでセットする
        GPD.NumSubobjects = static_cast<UINT>(std::size(SSs_GP)); GPD.ppSubobjects = std::data(SSs_GP);

        const D3D12_STATE_OBJECT_DESC SOD = {
            .Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
            .NumSubobjects = static_cast<UINT>(std::size(SSs)), .pSubobjects = std::data(SSs)
        };
        CComPtr<ID3D12StateObject> StateObject;
        VERIFY_SUCCEEDED(Device14->CreateStateObject(&SOD, IID_PPV_ARGS(&StateObject)));

        //!< 描画コマンド
        const auto Index = SwapChain4->GetCurrentBackBufferIndex();
        {
            const auto GCL10 = GraphicsCommandList10s[Index];
            VERIFY_SUCCEEDED(GCL10->Reset(CommandAllocator, nullptr));
            {
                GCL10->SetGraphicsRootSignature(RootSignature);

                //!< プログラムをセット
                CComPtr<ID3D12StateObjectProperties1> SOP1;
                SOP1 = StateObject;
                const D3D12_SET_PROGRAM_DESC SPD = {
                    .Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
                    .GenericPipeline = D3D12_SET_GENERIC_PIPELINE_DESC({.ProgramIdentifier = SOP1->GetProgramIdentifier(GPD.ProgramName) }),
                };
                GCL10->SetProgram(&SPD);

                GCL10->RSSetViewports(static_cast<UINT>(std::size(Viewports)), std::data(Viewports));
                GCL10->RSSetScissorRects(static_cast<UINT>(std::size(ScissorRects)), std::data(ScissorRects));

                auto SCRes = SwapChainResources[Index];
                const auto SCHandle = SwapChainHandles[Index];
                Barrier(GCL10, SCRes, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                {
                    const std::array RTVHandles = { SCHandle };
                    GCL10->OMSetRenderTargets(static_cast<UINT>(std::size(RTVHandles)), std::data(RTVHandles), FALSE, nullptr);

                    constexpr std::array ClearColor = { 0.529411793f, 0.807843208f, 0.921568692f, 1.0f };
                    GCL10->ClearRenderTargetView(SCHandle, std::data(ClearColor), 0, nullptr);

                    GCL10->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                    const std::array VBVs = { VertexBufferView };
                    GCL10->IASetVertexBuffers(0, static_cast<UINT>(std::size(VBVs)), std::data(VBVs));
                    GCL10->IASetIndexBuffer(&IndexBufferView);
                   
                    GCL10->ExecuteIndirect(CommandSignature, 1, IndirectBuffer, 0, nullptr, 0);
                }
                Barrier(GCL10, SCRes, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            }
            VERIFY_SUCCEEDED(GCL10->Close());
        }
    }
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            SendMessage(hWnd, WM_DESTROY, 0, 0);
            break;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            {
                WaitForFence(Fence, CommandQueue);

                const std::array CLs = { static_cast<ID3D12CommandList*>(GraphicsCommandList10s[SwapChain4->GetCurrentBackBufferIndex()]) };
                CommandQueue->ExecuteCommandLists(static_cast<UINT>(std::size(CLs)), std::data(CLs));

                VERIFY_SUCCEEDED(SwapChain4->Present(1, 0));
            }
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        {
            WaitForFence(Fence, CommandQueue);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
