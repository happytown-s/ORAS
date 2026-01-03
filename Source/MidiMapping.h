/*
  ==============================================================================

    MidiMapping.h
    Created: 3 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>

// =====================================================
// MIDIマッピング情報の構造体
// =====================================================
struct MidiMapping
{
    juce::String controlId;      // UIコントロールの一意識別子（例: "transport_play"）
    int midiChannel;              // MIDIチャンネル (0-15)
    int ccNumber;                 // CC番号 or Note番号 (0-127)
    bool isNote;                  // NoteメッセージかCCか
    float minValue;               // マッピング範囲の最小値
    float maxValue;               // マッピング範囲の最大値
    
    // デフォルトコンストラクタ
    MidiMapping()
        : midiChannel(0), ccNumber(0), isNote(false), minValue(0.0f), maxValue(1.0f)
    {}
    
    // パラメータ付きコンストラクタ
    MidiMapping(const juce::String& id, int channel, int cc, bool note = false, 
                float min = 0.0f, float max = 1.0f)
        : controlId(id), midiChannel(channel), ccNumber(cc), isNote(note), 
          minValue(min), maxValue(max)
    {}
    
    // JSON形式に変換
    juce::var toJSON() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("controlId", controlId);
        obj->setProperty("midiChannel", midiChannel);
        obj->setProperty("ccNumber", ccNumber);
        obj->setProperty("isNote", isNote);
        obj->setProperty("minValue", minValue);
        obj->setProperty("maxValue", maxValue);
        return juce::var(obj);
    }
    
    // JSONから復元
    static MidiMapping fromJSON(const juce::var& json)
    {
        MidiMapping mapping;
        if (auto* obj = json.getDynamicObject())
        {
            mapping.controlId = obj->getProperty("controlId").toString();
            mapping.midiChannel = obj->getProperty("midiChannel");
            mapping.ccNumber = obj->getProperty("ccNumber");
            mapping.isNote = obj->getProperty("isNote");
            mapping.minValue = obj->getProperty("minValue");
            mapping.maxValue = obj->getProperty("maxValue");
        }
        return mapping;
    }
    
    // MIDI値を正規化された値に変換（0-127 → minValue-maxValue）
    float convertMidiValue(int midiValue) const
    {
        float normalized = juce::jlimit(0.0f, 1.0f, midiValue / 127.0f);
        return minValue + normalized * (maxValue - minValue);
    }
    
    // 比較演算子
    bool operator==(const MidiMapping& other) const
    {
        return controlId == other.controlId;
    }
};
