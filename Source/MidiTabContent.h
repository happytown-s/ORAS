/*
  ==============================================================================

    MidiTabContent.h
    Created: 3 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "MidiLearnManager.h"
#include "ThemeColours.h"

// =====================================================
// MIDI設定タブのコンテンツ
// =====================================================
class MidiTabContent : public juce::Component, 
                       public juce::Timer,
                       public MidiLearnManager::Listener,
                       public juce::TableListBoxModel
{
public:
    MidiTabContent(MidiLearnManager& manager);
    ~MidiTabContent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // MidiLearnManager::Listener
    void midiMappingCreated(const MidiMapping& mapping) override;
    void midiMappingRemoved(const juce::String& controlId) override;
    void midiValueReceived(const juce::String& controlId, float value) override;
    void midiLearnModeChanged(bool isActive) override;
    void midiMessageReceived(const juce::String& description) override;
    
    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;

private:
    MidiLearnManager& midiManager;
    
    // MIDIデバイスセクション
    juce::Label deviceHeader;
    juce::OwnedArray<juce::ToggleButton> deviceToggles;
    juce::OwnedArray<juce::Label> deviceLabels;
    
    // MIDI Learnセクション
    juce::Label learnHeader;
    juce::ToggleButton learnModeToggle;
    juce::Label learnStatusLabel;
    
    // マッピング一覧セクション
    juce::Label mappingHeader;
    juce::TableListBox mappingTable;
    juce::TextButton clearAllButton;
    std::vector<MidiMapping> currentMappings;
    
    // MIDIモニターセクション
    juce::Label monitorHeader;
    juce::ToggleButton showMonitorToggle;
    juce::TextEditor monitorDisplay;
    std::deque<juce::String> monitorMessages;
    const int MAX_MONITOR_MESSAGES = 50;
    
    void updateDeviceList();
    void updateMappingTable();
    void addMonitorMessage(const juce::String& message);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiTabContent)
};
