// WorkGraphs.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WorkGraphs.h"

#define MAX_LOADSTRING 100

#include <array>
#include <algorithm>
#include <format>
#include <filesystem>

#include <d3d12.h>
#include <atlbase.h>
#include <d3dcompiler.h>
//#include <dxcapi.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

//!< DirectX ランタイム(DLL) のバージョンとその場所を指定する必要がある (NuGet でのデフォルトインストール先は ".\\D3D12\\" となる)
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 613; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = reinterpret_cast<const char*>(u8".\\D3D12\\"); }

#ifdef _DEBUG
#define VERIFY_SUCCEEDED(X) { const auto _HR = (X); if(FAILED(_HR)) { MessageBoxA(nullptr, std::data(std::system_category().message(_HR)), "", MB_OK); __debugbreak(); } }
#else
#define VERIFY_SUCCEEDED(X) (X) 
#endif

template<typename T> static constexpr size_t TotalSizeOf(const std::vector<T>& rhs) { return sizeof(T) * size(rhs); }
template<typename T, size_t U> static constexpr size_t TotalSizeOf(const std::array<T, U>& rhs) { return sizeof(rhs); }

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
    LoadStringW(hInstance, IDC_WORKGRAPHS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WORKGRAPHS));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WORKGRAPHS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WORKGRAPHS);
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
        //!< デバイス
        CComPtr<ID3D12Device> Device;
        VERIFY_SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device)));
        CComPtr<ID3D12Device14> Device14;
        Device14 = Device;
        auto CreateBuffer = [&](const UINT64 Size, const D3D12_RESOURCE_FLAGS Flags, const D3D12_HEAP_TYPE Type, const D3D12_RESOURCE_STATES State, ID3D12Resource** Resource) {
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
        };
        auto CreateDefaultBuffer = [&](const UINT64 Size, ID3D12Resource** Resource) { CreateBuffer(Size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, Resource); };
        auto CreateUABuffer = [&](const UINT64 Size, ID3D12Resource** Resource) { CreateBuffer(Size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, Resource); };
        auto CreateUploadBuffer = [&](const UINT64 Size, ID3D12Resource** Resource) { CreateBuffer(Size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON, Resource); };
        auto CreateReadbackBuffer = [&](const UINT64 Size, ID3D12Resource** Resource) { CreateBuffer(Size, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COMMON, Resource); };

        //!< WorkGraphs のサポートをチェック
        D3D12_FEATURE_DATA_D3D12_OPTIONS21 Options = {};
        VERIFY_SUCCEEDED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Options, sizeof(Options)));
        if (D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED == Options.WorkGraphsTier) {
            __debugbreak();
        }

        //!< コマンドキュー
        CComPtr<ID3D12CommandQueue> CQ;
        constexpr D3D12_COMMAND_QUEUE_DESC CQD = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0
        };
        VERIFY_SUCCEEDED(Device->CreateCommandQueue(&CQD, IID_PPV_ARGS(&CQ)));

        //!< フェンス
        CComPtr<ID3D12Fence> Fence;
        VERIFY_SUCCEEDED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
        auto WaitForFence = [&]() {
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
        };

        //!< コマンド
        CComPtr<ID3D12CommandAllocator> CA;
        VERIFY_SUCCEEDED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CA)));
        CComPtr<ID3D12CommandList> CL;
        VERIFY_SUCCEEDED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CA, nullptr, IID_PPV_ARGS(&CL)));
        CComPtr<ID3D12GraphicsCommandList10> GCL10;
        GCL10 = CL;
        VERIFY_SUCCEEDED(GCL10->Close()); //!< クローズしておく
        auto Barrier = [&](ID3D12Resource* Resource, const D3D12_RESOURCE_STATES Before, const D3D12_RESOURCE_STATES After) {
            const std::array RBs = {
              D3D12_RESOURCE_BARRIER({
                  .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                  .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                  .Transition = D3D12_RESOURCE_TRANSITION_BARRIER({
                      .pResource = Resource,
                      .Subresource = 0,
                      .StateBefore = Before,
                      .StateAfter = After,
                  }),
              }),
            };
            GCL10->ResourceBarrier(static_cast<UINT>(std::size(RBs)), std::data(RBs));
        };

        //!< シェーダ
        CComPtr<ID3DBlob> ShaderBlob;
        //!< (ここでは) オフラインコンパイルとする
        VERIFY_SUCCEEDED(D3DReadFileToBlob(std::data((std::filesystem::path(".") / std::string("WorkGraphs.sco")).wstring()), &ShaderBlob));
        constexpr std::array<D3D12_EXPORT_DESC, 0> EDs = {};
        const auto DLD = D3D12_DXIL_LIBRARY_DESC({
            .DXILLibrary = D3D12_SHADER_BYTECODE({
                .pShaderBytecode = ShaderBlob->GetBufferPointer(), .BytecodeLength = ShaderBlob->GetBufferSize() 
            }),
            .NumExports = static_cast<UINT>(std::size(EDs)), .pExports = std::data(EDs)
        });
//#define USE_COLLECTION
#ifdef USE_COLLECTION
        //!< TYPE_DXIL_LIBRARY をサブオブジェクトとして含む、TYPE_COLLECTION ステートオブジェクトを作成
        CComPtr<ID3D12StateObject> SO_Collection;
        {
            const std::array SSs = {
               D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &DLD }),
            };
            const D3D12_STATE_OBJECT_DESC SOD = {
              .Type = D3D12_STATE_OBJECT_TYPE_COLLECTION,
              .NumSubobjects = static_cast<UINT>(std::size(SSs)), .pSubobjects = std::data(SSs)
            };
            VERIFY_SUCCEEDED(Device14->CreateStateObject(&SOD, IID_PPV_ARGS(&SO_Collection)));
        }
        //!< TYPE_EXISTING_COLLECTION のサブオブジェクトとして、メインのステートオブジェクトへ追加する
        const D3D12_EXISTING_COLLECTION_DESC ECD = {
            .pExistingCollection = SO_Collection,
            .NumExports = static_cast<UINT>(std::size(EDs)), .pExports = std::data(EDs)
        };
#endif

        //!< ルートシグネチャ
        CComPtr<ID3D12RootSignature> RS;
#if true
        //!< Device14 では CreateRootSignatureFromSubobjectInLibrary() でルートシグネチャを作成できる
        VERIFY_SUCCEEDED(Device14->CreateRootSignatureFromSubobjectInLibrary(0, DLD.DXILLibrary.pShaderBytecode, DLD.DXILLibrary.BytecodeLength, L"GlobalRS", IID_PPV_ARGS(&RS)));
#else
        const std::array RPs = {
            D3D12_ROOT_PARAMETER1({
                .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
                .Descriptor = D3D12_ROOT_DESCRIPTOR1({ 
                    .ShaderRegister = 0, 
                    .RegisterSpace = 0, 
                    .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE
                })
            }),
        };
        constexpr std::array<D3D12_STATIC_SAMPLER_DESC, 0> SSDs = {};
        const D3D12_ROOT_SIGNATURE_DESC1 RSD = {
            .NumParameters = static_cast<UINT>(std::size(RPs)), .pParameters = std::data(RPs),
            .NumStaticSamplers = static_cast<UINT>(std::size(SSDs)), .pStaticSamplers = std::data(SSDs),
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE
        };
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC VRSD = { .Version = D3D_ROOT_SIGNATURE_VERSION_1_1, .Desc_1_1 = RSD, };
        CComPtr<ID3DBlob> RSBlob;
        CComPtr<ID3DBlob> ErrorBlob;
        VERIFY_SUCCEEDED(D3D12SerializeVersionedRootSignature(&VRSD, &RSBlob, &ErrorBlob));
        VERIFY_SUCCEEDED(Device->CreateRootSignature(0, RSBlob->GetBufferPointer(), RSBlob->GetBufferSize(), IID_PPV_ARGS(&RS)));
#endif
        const D3D12_GLOBAL_ROOT_SIGNATURE GRS = { .pGlobalRootSignature = RS };

        //!< ワークグラフ
        constexpr std::array<D3D12_NODE_ID, 0> NodeIDs = {};
        constexpr std::array<D3D12_NODE, 0> Nodes = {};
        const D3D12_WORK_GRAPH_DESC WGD = {
            .ProgramName = L"WorkGraphsName",
            .Flags = D3D12_WORK_GRAPH_FLAG_INCLUDE_ALL_AVAILABLE_NODES,
            .NumEntrypoints = static_cast<UINT>(std::size(NodeIDs)), .pEntrypoints = std::data(NodeIDs),
            .NumExplicitlyDefinedNodes = static_cast<UINT>(std::size(Nodes)), .pExplicitlyDefinedNodes = std::data(Nodes),
        };

        //!< ステートオブジェクト
        CComPtr<ID3D12StateObject> SO;
        const std::array SSs = {
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &GRS }),
#ifdef USE_COLLECTION
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION, .pDesc = &ECD }),
#else
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &DLD }),
#endif
            D3D12_STATE_SUBOBJECT({.Type = D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH, .pDesc = &WGD }),
        };
        const D3D12_STATE_OBJECT_DESC SOD = {
            .Type = D3D12_STATE_OBJECT_TYPE_EXECUTABLE,
            .NumSubobjects = static_cast<UINT>(std::size(SSs)), .pSubobjects = std::data(SSs)
        };
        VERIFY_SUCCEEDED(Device14->CreateStateObject(&SOD, IID_PPV_ARGS(&SO)));

        //!< バッキングメモリ
        CComPtr<ID3D12Resource> BackingMemory;
        //!< ローカルルート
        CComPtr<ID3D12Resource> LocalRootArgs;
        struct LOCAL_ROOT_ARGS { UINT Value; };
        std::vector<LOCAL_ROOT_ARGS> LocalRootArgsData;
        {
            CComPtr<ID3D12WorkGraphProperties> WGP;
            WGP = SO;
            const auto WGIndex = WGP->GetWorkGraphIndex(WGD.ProgramName);

            //!< プロパティの列挙
            {
                OutputDebugStringA(std::data(std::format("NumNodes = {}\n", WGP->GetNumNodes(WGIndex))));
                for (UINT i = 0; i < WGP->GetNumNodes(WGIndex); ++i) {
                    //!< ノード名と、エントリポイントインデックス
                    const auto NodeId = WGP->GetNodeID(WGIndex, i);
                    OutputDebugStringW(std::data(std::format(L"\t{}, {}\n", NodeId.Name, NodeId.ArrayIndex)));

                    //!< ローカルルート
                    const auto NLRATI = WGP->GetNodeLocalRootArgumentsTableIndex(WGIndex, i);
                    if (-1 != NLRATI) {
                        //!< (ここでは) 単にインデックスを取得する
                        LocalRootArgsData.emplace_back(LOCAL_ROOT_ARGS({ .Value = NLRATI }));
                        OutputDebugStringA(std::data(std::format("\tLocalRootArgs, {}\n", NLRATI)));
                    }
                }

                //!< エントリポイントとなるノードや明示的に [NodeIsProgramEntry] を指定したノードが列挙される
                OutputDebugStringA(std::data(std::format("NumEntrypoints = {}\n", WGP->GetNumEntrypoints(WGIndex))));
                for (UINT i = 0; i < WGP->GetNumEntrypoints(WGIndex); ++i) {
                    const auto NodeId = WGP->GetEntrypointID(WGIndex, i);
                    OutputDebugStringW(std::data(std::format(L"\t{}, {}\n", NodeId.Name, NodeId.ArrayIndex)));
                }
            }

            //!< バッキングメモリ
            D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS WGMR;
            WGP->GetWorkGraphMemoryRequirements(WGIndex, &WGMR);
            CreateDefaultBuffer(WGMR.MaxSizeInBytes, &BackingMemory);

            //!< ローカルルート
            if (!std::empty(LocalRootArgsData)) {
                CreateDefaultBuffer(TotalSizeOf(LocalRootArgsData), &LocalRootArgs);

                CComPtr<ID3D12Resource> Upload;
                CreateUploadBuffer(TotalSizeOf(LocalRootArgsData), &Upload);

                //VERIFY_SUCCEEDED(GCL10->Reset(CA, nullptr)); {
                //    //!< バリア COMMON -> COPY_DEST
                //    Barrier(UAV, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                //    {
                //    }
                //    //!< バリア COPY_DEST -> COMMON
                //    Barrier(UAV, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
                //}
                //VERIFY_SUCCEEDED(GCL10->Close());
            }
        }

        //!< 結果格納用 UAV
        CComPtr<ID3D12Resource> UAV;
        constexpr std::array UAVData = { UINT(0),UINT(0), UINT(0), UINT(0), UINT(0), UINT(0), UINT(0), UINT(0) };
        {
            CreateUABuffer(TotalSizeOf(UAVData), &UAV);
#if false
            //!< アップロード
            {
                //!< ステージングリソース
                CComPtr<ID3D12Resource> Upload;
                CreateUploadBuffer(TotalSizeOf(UAVData), &Upload);
                VERIFY_SUCCEEDED(GCL10->Reset(CA, nullptr)); {
                    //!< バリア COMMON -> COPY_DEST
                    Barrier(UAV, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                    {
                    }
                    //!< バリア COPY_DEST -> COMMON
                    Barrier(UAV, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
                }
                VERIFY_SUCCEEDED(GCL10->Close());
            }
#endif
        }
    
        //!< コマンド作成
        VERIFY_SUCCEEDED(GCL10->Reset(CA, nullptr));
        {
            //!< ルートシグネチャと UAV
            GCL10->SetComputeRootSignature(RS);
            GCL10->SetComputeRootUnorderedAccessView(0, UAV->GetGPUVirtualAddress());

            //!< プログラムをセット
            CComPtr<ID3D12StateObjectProperties1> SOP1;
            SOP1 = SO;
            const D3D12_SET_PROGRAM_DESC SPD = {
                .Type = D3D12_PROGRAM_TYPE_WORK_GRAPH,
                .WorkGraph = D3D12_SET_WORK_GRAPH_DESC({
                    .ProgramIdentifier = SOP1->GetProgramIdentifier(WGD.ProgramName),
                    .Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE,
                    .BackingMemory = D3D12_GPU_VIRTUAL_ADDRESS_RANGE({.StartAddress = BackingMemory->GetGPUVirtualAddress(), .SizeInBytes = BackingMemory->GetDesc().Width }),
                    .NodeLocalRootArgumentsTable = 
                        nullptr != LocalRootArgs ?
                        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE({.StartAddress = LocalRootArgs->GetGPUVirtualAddress(), .SizeInBytes = LocalRootArgs->GetDesc().Width, .StrideInBytes = sizeof(LocalRootArgsData[0])}) :
                        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE({.StartAddress = 0, .SizeInBytes = 0, .StrideInBytes = 0}),
                }),
            };
            GCL10->SetProgram(&SPD);

            //!< ディスパッチ
            struct EntryRecord
            {
                UINT GridSize;
                UINT RecordIndex;
            };
            //!< (ここでは) 4 ディスパッチ、それぞれのグリッドサイズは 1, 2, 3, 4 となる
            constexpr std::array InputData = {
                EntryRecord({.GridSize = 1, .RecordIndex = 0}),
                EntryRecord({.GridSize = 2, .RecordIndex = 1}),
                EntryRecord({.GridSize = 3, .RecordIndex = 2}),
                EntryRecord({.GridSize = 4, .RecordIndex = 3}),
            };
            const D3D12_DISPATCH_GRAPH_DESC DGD = {
                .Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
                .NodeCPUInput = D3D12_NODE_CPU_INPUT({
                    .EntrypointIndex = 0, //!< [NodeID("FirstNode", 0)]、[NodeID("FirstNode", 1)] のどちらを使用するか
                    .NumRecords = static_cast<UINT>(std::size(InputData)), .pRecords = std::data(InputData),
                    .RecordStrideInBytes = sizeof(InputData[0]),
                }),
            };
            GCL10->DispatchGraph(&DGD);
        } 
        VERIFY_SUCCEEDED(GCL10->Close());

        //!< コマンド発行
        OutputDebugString(L"Execute\n");
        const std::array CLs = { static_cast<ID3D12CommandList*>(GCL10)};
        CQ->ExecuteCommandLists(static_cast<UINT>(std::size(CLs)), std::data(CLs));

        //!< 待ち
        WaitForFence();
        
        OutputDebugString(L"Readback\n");
        //!< リードバック
        {
            //!< UAV リードバック用
            CComPtr<ID3D12Resource> UAReadback;
            CreateReadbackBuffer(TotalSizeOf(UAVData), &UAReadback);

            //!< コマンド作成
            VERIFY_SUCCEEDED(GCL10->Reset(CA, nullptr));
            {
                //!< バリア UNORDERED_ACCESS -> COPY_SOURCE
                Barrier(UAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

                //!< リードバックへコピー
                GCL10->CopyResource(UAReadback, UAV);
            }
            VERIFY_SUCCEEDED(GCL10->Close());

            //!< コマンド発行
            const std::array CLs = { static_cast<ID3D12CommandList*>(GCL10) };
            CQ->ExecuteCommandLists(static_cast<UINT>(std::size(CLs)), std::data(CLs));

            //!< 待ち
            WaitForFence();
 
            //!< 結果取得
            constexpr D3D12_RANGE Range = { .Begin = 0, .End = TotalSizeOf(UAVData) };
            UINT* Output;
            VERIFY_SUCCEEDED(UAReadback->Map(0, &Range, reinterpret_cast<void**>(&Output)));
            {
                for (auto i = 0; i < std::size(UAVData); ++i) {
                    OutputDebugStringA(std::data(std::format("\tUAV[{}] = {}\n", i, Output[i])));
                }
            }
            UAReadback->Unmap(0, nullptr);
        }
        OutputDebugString(L"Done\n");
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
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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
