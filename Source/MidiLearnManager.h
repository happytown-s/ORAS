/*
  ==============================================================================

    MidiLearnManager.h
    Created: 3 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include "MidiMapping.h"

// =====================================================
// MIDI Learn機能の中核マネージャークラス
// =====================================================
class MidiLearnManager : public juce::MidiInputCallback
{
public:
    MidiLearnManager();
    ~MidiLearnManager() override;
    
    // =====================================================
    // MIDIデバイス管理
    // =====================================================
    
    // 利用可能なMIDIデバイスのリストを取得
    juce::StringArray getAvailableMidiDevices();
    
    // 指定したMIDIデバイスを有効/無効にする
    void enableMidiDevice(const juce::String& deviceName, bool enable);
    
    // 現在有効なMIDIデバイスの一覧を取得
    juce::StringArray getEnabledDevices() const;
    
    // =====================================================
    // MIDI Learnモード
    // =====================================================
    
    // MIDI Learnモードを有効/無効にする
    void setLearnMode(bool enabled);
    
    // MIDI Learnモードが有効かどうか
    bool isLearnModeActive() const { return learnModeEnabled; }
    
    // 現在学習対象として選択されているコントロールIDを設定
    void setLearnTarget(const juce::String& controlId);
    
    // 現在の学習対象コントロールIDを取得
    juce::String getLearnTarget() const { return currentLearnTarget; }
    
    // 学習対象をクリア
    void clearLearnTarget() { currentLearnTarget.clear(); }
    
    // =====================================================
    // マッピング管理
    // =====================================================
    
    // 新しいマッピングを追加（同じcontrolIdがあれば上書き）
    bool addMapping(const MidiMapping& mapping);
    
    // 指定したコントロールIDのマッピングを削除
    void removeMapping(const juce::String& controlId);
    
    // すべてのマッピングをクリア
    void clearAllMappings();
    
    // 指定したコントロールIDのマッピングを検索
    MidiMapping* findMapping(const juce::String& controlId);
    
    // 指定したMIDI信号に対応するマッピングを検索
    MidiMapping* findMappingByMidi(int channel, int ccNumber, bool isNote);
    
    // すべてのマッピングを取得
    std::vector<MidiMapping> getAllMappings() const;
    
    // マッピングが存在するかチェック
    bool hasMapping(const juce::String& controlId) const;
    
    // =====================================================
    // MIDI入力処理
    // =====================================================
    
    // MIDIメッセージを受信（juce::MidiInputCallbackのオーバーライド）
    void handleIncomingMidiMessage(juce::MidiInput* source, 
                                   const juce::MidiMessage& message) override;
    
    // =====================================================
    // リスナー（UIへの通知用）
    // =====================================================
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        // 新しいマッピングが作成された時
        virtual void midiMappingCreated(const MidiMapping& mapping) {}
        
        // マッピングが削除された時
        virtual void midiMappingRemoved(const juce::String& controlId) {}
        
        // MIDI値を受信した時（通常モード）
        virtual void midiValueReceived(const juce::String& controlId, float value) {}
        
        // MIDI Learnモードが変更された時
        virtual void midiLearnModeChanged(bool isActive) {}
        
        // MIDI信号を受信した時（モニター用）
        virtual void midiMessageReceived(const juce::String& description) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // =====================================================
    // 永続化
    // =====================================================
    
    // マッピングをJSONファイルに保存
    bool saveToFile(const juce::File& file);
    
    // JSONファイルからマッピングを読み込み
    bool loadFromFile(const juce::File& file);

private:
    // マッピングテーブル（controlId → MidiMapping）
    std::map<juce::String, MidiMapping> mappings;
    
    // 有効なMIDI入力デバイス
    juce::OwnedArray<juce::MidiInput> activeInputs;
    juce::StringArray enabledDeviceNames;
    
    // リスナーリスト
    juce::ListenerList<Listener> listeners;
    
    // MIDI Learnモード状態
    bool learnModeEnabled = false;
    juce::String currentLearnTarget;
    
    // スレッドセーフのための排他制御
    juce::CriticalSection mappingLock;
    
    // 内部ヘルパー関数
    void notifyMappingCreated(const MidiMapping& mapping);
    void notifyMappingRemoved(const juce::String& controlId);
    void notifyValueReceived(const juce::String& controlId, float value);
    void notifyLearnModeChanged(bool isActive);
    void notifyMidiMessage(const juce::String& description);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnManager)
};
