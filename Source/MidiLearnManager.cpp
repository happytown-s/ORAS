/*
  ==============================================================================

    MidiLearnManager.cpp
    Created: 3 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#include "MidiLearnManager.h"

// =====================================================
// コンストラクタ / デストラクタ
// =====================================================

MidiLearnManager::MidiLearnManager()
{
    // 利用可能なMIDIデバイスを取得して初期化
    auto devices = juce::MidiInput::getAvailableDevices();
    DBG("Available MIDI devices: " + juce::String(devices.size()));
    
    for (const auto& device : devices)
    {
        DBG("  - " + device.name);
    }
}

MidiLearnManager::~MidiLearnManager()
{
    // すべてのMIDI入力を停止
    activeInputs.clear();
}

// =====================================================
// MIDIデバイス管理
// =====================================================

juce::StringArray MidiLearnManager::getAvailableMidiDevices()
{
    juce::StringArray deviceNames;
    auto devices = juce::MidiInput::getAvailableDevices();
    
    for (const auto& device : devices)
    {
        deviceNames.add(device.name);
    }
    
    return deviceNames;
}

void MidiLearnManager::enableMidiDevice(const juce::String& deviceName, bool enable)
{
    if (enable)
    {
        // すでに有効になっているかチェック
        if (enabledDeviceNames.contains(deviceName))
            return;
        
        // デバイスを開く
        auto devices = juce::MidiInput::getAvailableDevices();
        for (const auto& device : devices)
        {
            if (device.name == deviceName)
            {
                auto input = juce::MidiInput::openDevice(device.identifier, this);
                if (input != nullptr)
                {
                    input->start();
                    activeInputs.add(input.release());
                    enabledDeviceNames.add(deviceName);
                    DBG("MIDI device enabled: " + deviceName);
                }
                else
                {
                    DBG("Failed to open MIDI device: " + deviceName);
                }
                break;
            }
        }
    }
    else
    {
        // デバイスを無効化
        int index = enabledDeviceNames.indexOf(deviceName);
        if (index >= 0)
        {
            activeInputs.remove(index);
            enabledDeviceNames.remove(index);
            DBG("MIDI device disabled: " + deviceName);
        }
    }
}

juce::StringArray MidiLearnManager::getEnabledDevices() const
{
    return enabledDeviceNames;
}

// =====================================================
// MIDI Learnモード
// =====================================================

void MidiLearnManager::setLearnMode(bool enabled)
{
    if (learnModeEnabled != enabled)
    {
        learnModeEnabled = enabled;
        
        // モードが無効になったら学習対象をクリア
        if (!enabled)
        {
            currentLearnTarget.clear();
        }
        
        notifyLearnModeChanged(enabled);
        DBG("MIDI Learn mode: " + juce::String(enabled ? "ON" : "OFF"));
    }
}

void MidiLearnManager::setLearnTarget(const juce::String& controlId)
{
    currentLearnTarget = controlId;
    DBG("MIDI Learn target set: " + controlId);
}

// =====================================================
// マッピング管理
// =====================================================

bool MidiLearnManager::addMapping(const MidiMapping& mapping)
{
    const juce::ScopedLock lock(mappingLock);
    
    if (mapping.controlId.isEmpty())
        return false;
    
    // 既存のマッピングを上書き
    mappings[mapping.controlId] = mapping;
    
    DBG("Mapping added: " + mapping.controlId + 
        " -> Ch:" + juce::String(mapping.midiChannel) + 
        " CC:" + juce::String(mapping.ccNumber) +
        (mapping.isNote ? " (Note)" : " (CC)"));
    
    notifyMappingCreated(mapping);
    return true;
}

void MidiLearnManager::removeMapping(const juce::String& controlId)
{
    const juce::ScopedLock lock(mappingLock);
    
    auto it = mappings.find(controlId);
    if (it != mappings.end())
    {
        mappings.erase(it);
        DBG("Mapping removed: " + controlId);
        notifyMappingRemoved(controlId);
    }
}

void MidiLearnManager::clearAllMappings()
{
    const juce::ScopedLock lock(mappingLock);
    
    mappings.clear();
    DBG("All MIDI mappings cleared");
}

MidiMapping* MidiLearnManager::findMapping(const juce::String& controlId)
{
    const juce::ScopedLock lock(mappingLock);
    
    auto it = mappings.find(controlId);
    if (it != mappings.end())
        return &(it->second);
    
    return nullptr;
}

MidiMapping* MidiLearnManager::findMappingByMidi(int channel, int ccNumber, bool isNote)
{
    const juce::ScopedLock lock(mappingLock);
    
    for (auto& pair : mappings)
    {
        auto& mapping = pair.second;
        if (mapping.midiChannel == channel && 
            mapping.ccNumber == ccNumber && 
            mapping.isNote == isNote)
        {
            return &mapping;
        }
    }
    
    return nullptr;
}

std::vector<MidiMapping> MidiLearnManager::getAllMappings() const
{
    const juce::ScopedLock lock(mappingLock);
    
    std::vector<MidiMapping> result;
    result.reserve(mappings.size());
    
    for (const auto& pair : mappings)
    {
        result.push_back(pair.second);
    }
    
    return result;
}

bool MidiLearnManager::hasMapping(const juce::String& controlId) const
{
    const juce::ScopedLock lock(mappingLock);
    return mappings.find(controlId) != mappings.end();
}

// =====================================================
// MIDI入力処理
// =====================================================

void MidiLearnManager::handleIncomingMidiMessage(juce::MidiInput* source, 
                                                 const juce::MidiMessage& message)
{
    // CC または Note On/Off のみ処理
    if (!message.isController() && !message.isNoteOn() && !message.isNoteOff())
        return;
    
    int channel = message.getChannel() - 1; // JUCE は 1-indexed
    int ccNumber = message.isController() ? message.getControllerNumber() : message.getNoteNumber();
    bool isNote = message.isNoteOn() || message.isNoteOff();
    int value = message.isController() ? message.getControllerValue() : message.getVelocity();
    
    // モニター通知
    juce::String description = "Ch " + juce::String(channel + 1) + ": ";
    if (isNote)
        description += (message.isNoteOn() ? "Note On " : "Note Off ") + juce::String(ccNumber) + ", Vel " + juce::String(value);
    else
        description += "CC " + juce::String(ccNumber) + " = " + juce::String(value);
    
    notifyMidiMessage(description);
    
    // MIDI Learnモードの処理
    if (learnModeEnabled && currentLearnTarget.isNotEmpty())
    {
        // Note Offは無視（Note Onでマッピング作成）
        if (message.isNoteOff())
            return;
        
        // 新しいマッピングを作成
        MidiMapping newMapping(currentLearnTarget, channel, ccNumber, isNote);
        addMapping(newMapping);
        
        // 学習対象をクリア
        currentLearnTarget.clear();
        
        DBG("MIDI Learn completed: " + description);
        return;
    }
    
    // 通常モード：マッピングを検索して値を通知
    auto* mapping = findMappingByMidi(channel, ccNumber, isNote);
    if (mapping != nullptr)
    {
        // Note Offは値0として処理（トグルボタン用）
        float normalizedValue = message.isNoteOff() ? 0.0f : mapping->convertMidiValue(value);
        
        notifyValueReceived(mapping->controlId, normalizedValue);
    }
}

// =====================================================
// リスナー管理
// =====================================================

void MidiLearnManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void MidiLearnManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

// =====================================================
// 通知ヘルパー関数
// =====================================================

void MidiLearnManager::notifyMappingCreated(const MidiMapping& mapping)
{
    juce::MessageManager::callAsync([this, mapping]()
    {
        listeners.call([&mapping](Listener& l) { l.midiMappingCreated(mapping); });
    });
}

void MidiLearnManager::notifyMappingRemoved(const juce::String& controlId)
{
    juce::MessageManager::callAsync([this, controlId]()
    {
        listeners.call([&controlId](Listener& l) { l.midiMappingRemoved(controlId); });
    });
}

void MidiLearnManager::notifyValueReceived(const juce::String& controlId, float value)
{
    juce::MessageManager::callAsync([this, controlId, value]()
    {
        listeners.call([&controlId, value](Listener& l) { l.midiValueReceived(controlId, value); });
    });
}

void MidiLearnManager::notifyLearnModeChanged(bool isActive)
{
    juce::MessageManager::callAsync([this, isActive]()
    {
        listeners.call([isActive](Listener& l) { l.midiLearnModeChanged(isActive); });
    });
}

void MidiLearnManager::notifyMidiMessage(const juce::String& description)
{
    juce::MessageManager::callAsync([this, description]()
    {
        listeners.call([&description](Listener& l) { l.midiMessageReceived(description); });
    });
}

// =====================================================
// 永続化
// =====================================================

bool MidiLearnManager::saveToFile(const juce::File& file)
{
    const juce::ScopedLock lock(mappingLock);
    
    // JSON配列を作成
    juce::var mappingsArray;
    juce::Array<juce::var> mappingList;
    
    for (const auto& pair : mappings)
    {
        mappingList.add(pair.second.toJSON());
    }
    
    mappingsArray = mappingList;
    
    // ルートオブジェクト作成
    auto* root = new juce::DynamicObject();
    root->setProperty("version", 1);
    root->setProperty("mappings", mappingsArray);
    root->setProperty("enabledDevices", enabledDeviceNames);
    
    // ファイルに書き込み
    juce::String jsonString = juce::JSON::toString(juce::var(root), true);
    
    if (file.replaceWithText(jsonString))
    {
        DBG("MIDI mappings saved to: " + file.getFullPathName());
        return true;
    }
    else
    {
        DBG("Failed to save MIDI mappings");
        return false;
    }
}

bool MidiLearnManager::loadFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("MIDI mapping file does not exist: " + file.getFullPathName());
        return false;
    }
    
    // ファイルを読み込み
    juce::String jsonString = file.loadFileAsString();
    juce::var json = juce::JSON::parse(jsonString);
    
    if (!json.isObject())
    {
        DBG("Invalid MIDI mapping file format");
        return false;
    }
    
    const juce::ScopedLock lock(mappingLock);
    
    // マッピングをクリア
    mappings.clear();
    
    // マッピングを復元
    if (auto* root = json.getDynamicObject())
    {
        auto mappingsVar = root->getProperty("mappings");
        if (mappingsVar.isArray())
        {
            auto* mappingsArray = mappingsVar.getArray();
            for (const auto& mappingVar : *mappingsArray)
            {
                auto mapping = MidiMapping::fromJSON(mappingVar);
                if (!mapping.controlId.isEmpty())
                {
                    mappings[mapping.controlId] = mapping;
                }
            }
        }
        
        // 有効デバイスを復元
        auto devicesVar = root->getProperty("enabledDevices");
        if (devicesVar.isArray())
        {
            auto* devicesArray = devicesVar.getArray();
            for (const auto& deviceVar : *devicesArray)
            {
                juce::String deviceName = deviceVar.toString();
                enableMidiDevice(deviceName, true);
            }
        }
    }
    
    DBG("MIDI mappings loaded: " + juce::String(mappings.size()) + " mappings");
    return true;
}
