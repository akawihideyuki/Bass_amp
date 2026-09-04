# Bass_amp ベースアンプシミュレーター 初版仕様書

## 1. 目的

Windows PCへ接続したエレキベースをリアルタイム処理し、低遅延で演奏できるスタンドアロン型ベースアンプシミュレーターを制作する。

単なるギターアンプの低音版ではなく、以下を重視する。

- ベースの低域を失わない
- 歪ませても音程感と芯が残る
- 指弾き・ピック・スラップで扱いやすい
- 接続してすぐ練習できる
- PCソフトならではの柔軟な音作り

GitHubで無料公開し、個人利用・実験を目的とする。

## 2. 対象環境

- Windows 11 64-bit
- スタンドアロンアプリ
- エレキベース -> USBオーディオインターフェース -> PC -> オーディオ出力を主用途とする
- 初版ではmacOS / Linux / VST3 / AU / AAXは対象外

## 3. 技術構成

- C++20
- CMake 3.22+
- JUCE 9.0.1
- Visual Studio 2022 / MSVC
- JUCE Native GUI
- AGPL-3.0-only

JUCEに音声デバイス、ASIO/WASAPI、DSP、GUIを集約し、複数フレームワーク間の橋渡しを減らして実装容易性を優先する。

## 4. オーディオI/O

正式対応:

1. ASIO
2. WASAPI

ASIO4ALLはアプリへ同梱せず、PCにインストールされた通常のASIOドライバーとして利用する。

推奨初期値:

- 48,000 Hz
- 128 samples
- Mono input
- Stereo output

利用機器が対応しない設定を強制しない。

## 5. 信号経路

```text
BASS INPUT
  -> INPUT GAIN
  -> NOISE GATE
  -> COMPRESSOR
  -> AMP ENGINE
  -> LOW CLEAN MIX
  -> 4 BAND EQ
  -> CABINET
  -> LIMITER
  -> MASTER
  -> OUTPUT
```

DSPモジュールはUIから独立させる。

## 6. Amp Engine

3種類のVoicingを搭載する。

### VINTAGE

- 丸い
- 太い
- 柔らかなコンプレッション感
- 中低域中心
- 強く弾いたとき自然に歪む

### MODERN

- タイト
- 高解像度
- 明瞭なアタック
- スラップにも対応

### AGGRESSIVE

- 強い倍音
- 中高域の存在感
- ピックとの相性重視
- ロック / メタル向け

3つの完全別DSPを作らず、共通Amp Engineへ異なるProfile/係数を与える。

## 7. Low Clean

ベース専用の中心機能。

Linkwitz-Riley crossoverを使って低域と高域を分け、歪ませた低域とクリーン低域を再合成する。

初期クロスオーバー目安: 120 Hz。

`LOW CLEAN` 0-100%で低域のクリーン比率を調整し、強いDriveでも低域の芯を維持する。

## 8. Character

Bass_amp独自コントロール。

0-100%で以下を連動させる。

- 倍音構成
- 中域の押し出し
- Pre-emphasis
- Saturation特性 / Bias

Voicingごとに効き方を変えてよい。

## 9. Gate / Compressor

JUCE標準DSPを基本利用する。

Gate通常UI:

- Threshold

Advanced候補:

- Ratio
- Attack
- Release

Compressor通常UI:

- COMP Amount

AmountからThreshold / Ratio / Attack / Releaseを実用範囲へマッピングし、通常操作を簡単にする。

## 10. EQ

4-band EQ:

- Bass: 80 Hz Low Shelf
- Low Mid: 250 Hz Bell
- High Mid: 1.2 kHz Bell
- Treble: 4 kHz High Shelf
- Gain: ±12 dB

周波数・Qは将来変更できるよう、ハードコードを散在させない。

## 11. Cabinet

初版内蔵Cabは実装容易性を優先して軽量フィルタ方式。

- OFF / DI
- Compact 1x15
- Punch 4x10
- Massive 8x10

実在メーカー名は使用しない。

将来 `juce::dsp::Convolution` を利用してUser IR WAV読み込みを追加する。

## 12. Limiter / Safety

最終段にLimiterを置き、爆音事故と異常出力を抑える。

必須安全策:

- Startup mute思想
- Device change時の安全な再初期化
- Parameter range clamp
- NaN / Infinity guard
- Output limiter
- Clip indication

極端な有効設定でもアプリが破綻してはならない。

## 13. UI

一画面で演奏に必要なものを操作できることを優先する。

通常画面の主要操作:

- Input
- Gate
- Comp
- Vintage / Modern / Aggressive
- Drive
- Character
- Low Clean
- Bass
- Low Mid
- High Mid
- Treble
- Cabinet
- Master
- Global Bypass
- Audio Setup
- Input / Output meters

高度な設定はAdvancedへ分離する。

## 14. Audio Setup

最低限変更可能にする項目:

- Driver Type
- Input Device
- Output Device
- Input Channel
- Output Channels
- Sample Rate
- Buffer Size

最後に成功した設定の保存は後続実装対象。

## 15. Tuner

Phase 1でYIN系Pitch Detectionを追加予定。

- B0程度から一般的なベース音域を対象
- A4基準ピッチ変更可
- Pitch解析をAudio Callback上で重く実行しない

## 16. Preset / A-B

Phase 1でJSON Presetを実装予定。

- `presetVersion` を必須保存
- Factory Preset
- User Save / Load
- A/B比較はDSP二重化ではなくParameter Stateを2セット保持

## 17. リアルタイム規則

Audio Callbackでは原則禁止:

- Heap allocation
- File I/O
- JSON処理
- GUI更新
- Disk logging
- Mutex wait
- IR loading

UI -> DSPのパラメータ共有はAtomic等を利用し、連続値はParameter Smoothingを使う。

## 18. Diagnostics

後続実装で以下を表示できるようにする。

- Driver
- Device
- Sample Rate
- Buffer Size
- Input / Output latency
- CPU usage
- Peak callback time
- Callback deadline miss count

ハードウェアで計測していない値を推測表示しない。

## 19. テスト

DSPは実ベースなしでも検証可能な構造にする。

入力候補:

- Silence
- Sine
- Impulse
- White noise
- Test WAV

最低確認:

- NaN / Infinityなし
- 異常振幅なし
- Bypassが成立
- 44.1 / 48 / 96 kHz
- 64 / 128 / 256 / 512 samples
- Device変更でクラッシュしない
- ASIOとWASAPI双方を壊さない

## 20. Phase

### Phase 0 - Audio Foundation

- JUCE app
- ASIO / WASAPI
- Device selector
- Input -> Output
- Gain
- Meter
- Mute / Bypass
- 48 kHz / 128 samplesを標準目標

### Phase 1 - Bass Amp Core

- Gate
- Compressor
- 3 Voicing
- Drive
- Character
- Low Clean
- 4-band EQ
- Cab profiles
- Limiter
- Tuner
- Preset
- A/B
- Diagnostics
- User IR

ここをv1.0完成目標とする。

### Phase 2 - Quality

- Amp DSP改善
- Cabinet IR強化
- Spectrum Analyzer
- Recording
- Additional Voicing
- UI polish

### Phase 3 - Expansion

- Chorus
- Octaver
- Fuzz
- Envelope Filter
- Looper
- VST3
- Neural Amp Model

## 21. 初版でやらないこと

- Neural amp capture
- DAW / Multitrack
- Cloud account
- Online preset service
- Auto updater
- macOS/Linux
- Plugin formats

## 22. v1.0 Done

- Windows 11で起動
- ASIO / ASIO4ALL / WASAPIを選択可能
- ベースをリアルタイム演奏可能
- 48 kHz / 128 samplesで安定動作する実機環境を確認
- Gate / Compressor / 3 Voicing / Drive / Character / Low Clean / EQ / Cab / Limiter動作
- Tuner動作
- Preset / A-B動作
- User IR読込
- Meter / Audio Setup / Diagnostics
- 異常入力でNaN/Infなし
- DSPテスト
- README / AGENTS / LICENSE整備

## 23. 最優先順位

1. 安定して音が鳴る
2. 低遅延
3. 爆音・音切れを避ける
4. ベースとして気持ちいい音
5. 操作性
6. CPU効率
7. 見た目
8. 追加機能

Bass_ampは「有名アンプを大量にコピーするソフト」ではなく、一本のベースをPCにつないですぐ良い音で弾ける、オリジナルの万能ベースアンプを目標とする。
