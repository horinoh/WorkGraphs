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
    ~~~
    $dxc.exe -E main -T lib_6_8 -Fo XXX.sco XXX.hlsl
    $dxc.exe -E main -T lib_6_8 -Fo XXX.sco XXX.hlsl -Zi -Qembed_debug
    ~~~
    - HLSL はビルド除外設定にしておき、Build Events - Pre Build Event へ dxc コマンドを記述しておく


