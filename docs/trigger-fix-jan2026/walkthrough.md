# トリガー鎮火・録音ギャップ修正ウォークスルー

## 日付
2026年1月2日

## 概要
キャリブレーション機能導入後に発生した2つの問題を修正しました。

---

## 問題1: トリガー鎮火（リセット）が機能しない

### 症状
音声入力でトリガーが発火した後、音が止まっても次のトリガーが発火しない。

### 原因
`detectMultiChannelTrigger` はカスタム閾値（キャリブレーション後の `effectiveThreshold`）で高閾値チェックを行っていましたが、信号がノイズフロアを下回っても `AudioInputBuffer::inPreRoll` がリセットされませんでした。

### 修正
1. **`AudioInputBuffer.h`**: `resetPreRoll()` メソッドを追加。
2. **`InputManager.cpp`**: `detectMultiChannelTrigger` の末尾で、全チャンネルのレベルが閾値の半分未満になった場合に `inputBuffer.resetPreRoll()` を呼び出すロジックを追加。

---

## 問題2: 録音完了後のギャップ

### 症状
録音完了後にループ再生を開始すると、頭に余計な無音が入る。

### 原因
`stopRecording` で `masterReadPosition = 0` にリセットしていましたが、`startPlaying` が呼ばれる瞬間には `masterReadPosition` が若干進んでいる可能性がありました。

### 修正
**`LooperAudio.cpp`**: `stopRecording` 内のマスタートラック作成時に、`track.readPosition = 0` を直接設定するように修正。

---

## 変更ファイル
- [AudioInputBuffer.h](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/AudioInputBuffer.h): `resetPreRoll()` メソッド追加
- [InputManager.cpp](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/InputManager.cpp): 鎮火ロジック追加
- [LooperAudio.cpp](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/LooperAudio.cpp): `readPosition` 初期化修正

## 検証方法
1. アプリを起動し、音声入力でトリガーを発火させる。
2. 音を止め、無音が続いた後に再度音を鳴らし、トリガーが再発火することを確認。
3. 録音完了後にループ再生を開始し、頭にギャップがないことを確認。
