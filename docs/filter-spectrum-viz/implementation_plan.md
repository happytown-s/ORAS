# フィルターエンベロープ＆スペクトラム可視化 実装プラン

## 目的
選択されているトラックの周波数スペクトラムをリアルタイムで表示し、その上にアクティブなフィルター（カットオフ/レゾナンス）の周波数応答カーブを重ねて表示します。

## ユーザーレビューが必要な事項
> [!NOTE]
> この機能は、可視化のためにリアルタイムのFFT処理を追加します。軽量ですが、UIスレッドに多少のCPU負荷がかかります。
> オーディオスレッドからUIスレッドへのスレッドセーフなデータ転送には `juce::AbstractFifo` を使用します。

## 変更予定の内容

### [LooperAudio](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/LooperAudio.h)
- **モニタリング用FIFOの追加**:
    - `juce::AbstractFifo monitorFifo { 4096 }`
    - `std::vector<float> monitorBuffer`
    - `std::atomic<int> monitorTrackId { -1 }`
- **オーディオ処理**:
    - `mixTracksToOutput` ループ内で `trackId == monitorTrackId` の場合、サンプルを `monitorFifo` にプッシュ。

### [NEW] [FilterSpectrumVisualizer](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/FilterSpectrumVisualizer.h)
- **コンポーネント**:
    - `juce::dsp::FFT forwardFFT` (1024 or 2048ポイント)
    - `paint(g)`:
        - 周波数グリッドの描画 (20Hz - 20kHz, 対数軸)。
        - FFTスペクトラムの描画 (グラデーション塗りつぶし)。
        - フィルターのマグニチュード応答曲線の描画 (太線)。
- **フィルターカーブの計算**:
    - State Variable Filter (LPF/HPF) のマグニチュード応答 $H(f)$ を計算。

### [FXPanel](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/FXPanel.cpp)
- **統合**:
    - `FilterSpectrumVisualizer` メンバを追加。
    - レイアウトに配置 (フィルタースライダーの上など)。
    - `timerCallback` 内で `LooperAudio` からデータを取得し `FilterSpectrumVisualizer` へ供給。

## 検証プラン
### 手動検証
- トラックを再生し、スペクトラムの動きを確認。
- フィルターのCutoff/Resonanceスライダーを動かし、白いカーブが即座に更新されることを確認。
- トラックを切り替え、スペクトラムのソースが新しいトラックに変更されることを確認。
