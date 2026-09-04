# Bass_amp

Windows 11向けの、エレキベースをPCへ接続してリアルタイム演奏するスタンドアロン・ベースアンプシミュレーターです。

## 現在の段階

最初の実装では、JUCEによる低遅延オーディオ基盤とベース向けコアDSPを構築しています。

- ASIO / ASIO4ALL（ASIOドライバーとして選択）
- WASAPI
- 48 kHz / 128 samplesを標準目標
- Input Gain
- Noise Gate
- Compressor
- Vintage / Modern / Aggressive Voicing
- Drive / Character / Low Clean
- 4-band EQ
- 軽量Cabシミュレーション（DI / 1x15 / 4x10 / 8x10）
- Output Limiter
- Input / Output meter
- JUCE Audio Device Setup

詳細仕様は [`docs/SPECIFICATION.md`](docs/SPECIFICATION.md) を参照してください。

## ビルド環境

- Windows 11 64-bit
- Visual Studio 2022（Desktop development with C++）
- CMake 3.22+
- Git
- インターネット接続（初回CMake configure時にJUCE 9.0.1を取得）

## ビルド

Developer PowerShell for VS 2022 等で実行します。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

生成物は通常 `build/BassAmp_artefacts/Release/` 以下に作成されます。

## オーディオ設定

アプリ上部の `AUDIO SETUP` からデバイスを設定します。

推奨開始値:

- 純正ASIOドライバーがある場合はASIOを優先
- Sample Rate: 48,000 Hz
- Buffer Size: 128 samples
- Input: ベースを接続したモノラル入力
- Output: 使用するヘッドホン / スピーカー出力

ASIO4ALLはBass_ampに同梱しません。PCへインストールされている場合はASIOドライバー一覧から選択します。

## ライセンス

Bass_ampは **AGPL-3.0-only** で公開します。

JUCEのオープンソースライセンスおよびASIO関連コードのライセンス条件にも従います。
