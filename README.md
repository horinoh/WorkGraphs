# WorkGraphs

## 準備
- NuGet Package で Microsoft.Direct3D.D3D12 DirectX 12 Agility SDK をインストールしておく
    -  DirectX ランタイムのバージョン、場所の指定を以下のように .cpp へ記述
    ~~~
    extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 613; }
    extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = reinterpret_cast<const char*>(u8".\\D3D12\\"); }
    ~~~
- GPU RTX30 シリーズ以降、NVIDIA ドライバ 551.76 以降 
- シェーダモデル 6.8 以降
    - オフラインコンパイルする場合 [dxc.exe](https://github.com/microsoft/DirectXShaderCompiler/releases) をインストールしておく
        - コンパイル例
        ~~~
        $dxc.exe -E FirstNode -T lib_6_8 -Fo XXX.sco XXX.hlsl
        $dxc.exe -E FirstNode -T lib_6_8 -Fo XXX.sco XXX.hlsl -Zi -Qembed_debug
        ~~~
    - HLSL は Item Type を Custom Build Tool にして、ビルド用のコマンドラインを記述した
    - dxc.exe のインストール先は、環境変数 DXC_PATH を作成してコマンドライン中で使用している
- atlbase.h が無いと怒られる場合
    - Visual Studio インストーラから「最新の vXXX ビルドツール用 C++ ATL (x86 および x64)」をインストールしておく

## 参考
- [DirectX-Specs](https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html)
- [WorkGraphs](https://devblogs.microsoft.com/directx/d3d12-work-graphs/#CoalescingLaunch)
- [サンプル](https://github.com/microsoft/DirectX-Graphics-Samples)
    - Samples - Desktop - D3D12HelloWorld - src - D3D12HelloWorld.sln
        - D3D12HelloWorkGraphs
        - D3D12WorkGraphsSandbox
        - D3D12HelloGenericPrograms

