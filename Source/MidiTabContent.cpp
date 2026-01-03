/*
  ==============================================================================

    MidiTabContent.cpp
    Created: 3 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#include "MidiTabContent.h"

MidiTabContent::MidiTabContent(MidiLearnManager& manager)
    : midiManager(manager), mappingTable({}, this)
{
    // ========== MIDIデバイスセクション ==========
    deviceHeader.setText("MIDI Devices", juce::dontSendNotification);
    deviceHeader.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    deviceHeader.setColour(juce::Label::textColourId, ThemeColours::Silver);
    addAndMakeVisible(deviceHeader);
    
    updateDeviceList();
    
    // ========== MIDI Learnセクション ==========
    learnHeader.setText("MIDI Learn", juce::dontSendNotification);
    learnHeader.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    learnHeader.setColour(juce::Label::textColourId, ThemeColours::Silver);
    addAndMakeVisible(learnHeader);
    
    learnModeToggle.setButtonText("Enable MIDI Learn Mode");
    learnModeToggle.setClickingTogglesState(true);
    learnModeToggle.setColour(juce::TextButton::buttonOnColourId, ThemeColours::NeonMagenta);
    learnModeToggle.onClick = [this]()
    {
        midiManager.setLearnMode(learnModeToggle.getToggleState());
    };
    addAndMakeVisible(learnModeToggle);
    
    learnStatusLabel.setText("Status: Ready", juce::dontSendNotification);
    learnStatusLabel.setColour(juce::Label::textColourId, ThemeColours::Silver);
    addAndMakeVisible(learnStatusLabel);
    
    // ========== マッピング一覧セクション ==========
    mappingHeader.setText("MIDI Mappings", juce::dontSendNotification);
    mappingHeader.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    mappingHeader.setColour(juce::Label::textColourId, ThemeColours::Silver);
    addAndMakeVisible(mappingHeader);
    
    // テーブルのカラム設定
    mappingTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    mappingTable.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    mappingTable.getHeader().addColumn("Control", 1, 200, 100, 400);
    mappingTable.getHeader().addColumn("Ch", 2, 40, 40, 60);
    mappingTable.getHeader().addColumn("CC/Note", 3, 80, 60, 100);
    mappingTable.getHeader().addColumn("Value", 4, 80, 60, 100);
    mappingTable.getHeader().addColumn("Delete", 5, 60, 60, 80);
    mappingTable.setOutlineThickness(1);
    addAndMakeVisible(mappingTable);
    
    clearAllButton.setButtonText("Clear All Mappings");
    clearAllButton.onClick = [this]()
    {
        midiManager.clearAllMappings();
        updateMappingTable();
    };
    addAndMakeVisible(clearAllButton);
    
    // ========== MIDIモニターセクション ==========
    monitorHeader.setText("MIDI Monitor", juce::dontSendNotification);
    monitorHeader.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    monitorHeader.setColour(juce::Label::textColourId, ThemeColours::Silver);
    addAndMakeVisible(monitorHeader);
    
    showMonitorToggle.setButtonText("Show");
    showMonitorToggle.setClickingTogglesState(true);
    showMonitorToggle.setToggleState(true, juce::dontSendNotification);
    showMonitorToggle.setColour(juce::TextButton::buttonOnColourId, ThemeColours::NeonCyan);
    showMonitorToggle.onClick = [this]()
    {
        monitorDisplay.setVisible(showMonitorToggle.getToggleState());
    };
    addAndMakeVisible(showMonitorToggle);
    
    monitorDisplay.setMultiLine(true);
    monitorDisplay.setReadOnly(true);
    monitorDisplay.setScrollbarsShown(true);
    monitorDisplay.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    monitorDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    monitorDisplay.setColour(juce::TextEditor::textColourId, ThemeColours::NeonCyan);
    addAndMakeVisible(monitorDisplay);
    
    // リスナー登録
    midiManager.addListener(this);
    
    // 初期化
    updateMappingTable();
    startTimerHz(10); // 定期更新
}

MidiTabContent::~MidiTabContent()
{
    midiManager.removeListener(this);
}

void MidiTabContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff151515));
}

void MidiTabContent::resized()
{
    auto area = getLocalBounds().reduced(15);
    
    // MIDIデバイスセクション
    deviceHeader.setBounds(area.removeFromTop(30));
    auto deviceArea = area.removeFromTop(80);
    
    int y = 0;
    for (int i = 0; i < deviceToggles.size(); ++i)
    {
        auto row = deviceArea.removeFromTop(25);
        deviceToggles[i]->setBounds(row.removeFromLeft(150));
        deviceLabels[i]->setBounds(row.removeFromLeft(150));
        y += 25;
    }
    
    area.removeFromTop(10);
    
    // MIDI Learnセクション
    learnHeader.setBounds(area.removeFromTop(30));
    learnModeToggle.setBounds(area.removeFromTop(30).reduced(3));
    learnStatusLabel.setBounds(area.removeFromTop(25));
    
    area.removeFromTop(10);
    
    // マッピング一覧セクション
    auto mappingRow = area.removeFromTop(30);
    mappingHeader.setBounds(mappingRow.removeFromLeft(200));
    clearAllButton.setBounds(mappingRow.removeFromRight(150).reduced(3));
    
    mappingTable.setBounds(area.removeFromTop(200));
    
    area.removeFromTop(10);
    
    // MIDIモニターセクション
    auto monitorRow = area.removeFromTop(30);
    monitorHeader.setBounds(monitorRow.removeFromLeft(150));
    showMonitorToggle.setBounds(monitorRow.removeFromLeft(80).reduced(3));
    
    monitorDisplay.setBounds(area.removeFromTop(100));
}

void MidiTabContent::timerCallback()
{
    // 定期的にデバイスリストを更新（デバイスの接続/切断を検出）
    // パフォーマンス考慮で1秒に1回程度に制限
    static int counter = 0;
    if (++counter >= 10)
    {
        updateDeviceList();
        counter = 0;
    }
}

// =====================================================
// MidiLearnManager::Listener
// =====================================================

void MidiTabContent::midiMappingCreated(const MidiMapping& mapping)
{
    updateMappingTable();
    addMonitorMessage("Mapping created: " + mapping.controlId);
}

void MidiTabContent::midiMappingRemoved(const juce::String& controlId)
{
    updateMappingTable();
    addMonitorMessage("Mapping removed: " + controlId);
}

void MidiTabContent::midiValueReceived(const juce::String& controlId, float value)
{
    // 値受信は頻繁なのでモニターには表示しない
}

void MidiTabContent::midiLearnModeChanged(bool isActive)
{
    learnModeToggle.setToggleState(isActive, juce::dontSendNotification);
    
    if (isActive)
        learnStatusLabel.setText("Status: Waiting for control selection...", juce::dontSendNotification);
    else
        learnStatusLabel.setText("Status: Ready", juce::dontSendNotification);
}

void MidiTabContent::midiMessageReceived(const juce::String& description)
{
    if (showMonitorToggle.getToggleState())
    {
        addMonitorMessage(description);
    }
}

// =====================================================
// TableListBoxModel
// =====================================================

int MidiTabContent::getNumRows()
{
    return (int)currentMappings.size();
}

void MidiTabContent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff2a2a2a));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff1a1a1a));
    else
        g.fillAll(juce::Colour(0xff151515));
}

void MidiTabContent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= currentMappings.size())
        return;
    
    const auto& mapping = currentMappings[rowNumber];
    g.setColour(ThemeColours::Silver);
    g.setFont(11.0f);
    
    juce::String text;
    switch (columnId)
    {
        case 1: // Control
            text = mapping.controlId;
            break;
        case 2: // Channel
            text = juce::String(mapping.midiChannel + 1);
            break;
        case 3: // CC/Note
            if (mapping.isNote)
                text = "Note " + juce::String(mapping.ccNumber);
            else
                text = "CC " + juce::String(mapping.ccNumber);
            break;
        case 4: // Value
            text = juce::String(mapping.minValue, 2) + " - " + juce::String(mapping.maxValue, 2);
            break;
        case 5: // Delete
            text = "×";
            g.setColour(ThemeColours::RecordingRed);
            g.setFont(16.0f);
            break;
    }
    
    g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
}

void MidiTabContent::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    if (columnId == 5 && rowNumber < currentMappings.size())
    {
        // 削除ボタンクリック
        const auto& mapping = currentMappings[rowNumber];
        midiManager.removeMapping(mapping.controlId);
    }
}

// =====================================================
// 内部ヘルパー関数
// =====================================================

void MidiTabContent::updateDeviceList()
{
    auto availableDevices = midiManager.getAvailableMidiDevices();
    auto enabledDevices = midiManager.getEnabledDevices();
    
    // 既存のデバイスリストと異なる場合のみ更新
    if (deviceToggles.size() != availableDevices.size())
    {
        deviceToggles.clear();
        deviceLabels.clear();
        
        for (const auto& deviceName : availableDevices)
        {
            auto* toggle = new juce::ToggleButton();
            toggle->setButtonText("");
            toggle->setToggleState(enabledDevices.contains(deviceName), juce::dontSendNotification);
            toggle->setColour(juce::TextButton::buttonOnColourId, ThemeColours::PlayingGreen);
            toggle->onClick = [this, deviceName, toggle]()
            {
                midiManager.enableMidiDevice(deviceName, toggle->getToggleState());
            };
            addAndMakeVisible(toggle);
            deviceToggles.add(toggle);
            
            auto* label = new juce::Label();
            label->setText(deviceName, juce::dontSendNotification);
            label->setColour(juce::Label::textColourId, ThemeColours::Silver);
            addAndMakeVisible(label);
            deviceLabels.add(label);
        }
        
        resized();
    }
}

void MidiTabContent::updateMappingTable()
{
    currentMappings = midiManager.getAllMappings();
    
    // アルファベット順にソート
    std::sort(currentMappings.begin(), currentMappings.end(), 
              [](const MidiMapping& a, const MidiMapping& b) {
                  return a.controlId < b.controlId;
              });
    
    mappingTable.updateContent();
}

void MidiTabContent::addMonitorMessage(const juce::String& message)
{
    monitorMessages.push_back(message);
    
    // 最大メッセージ数を超えたら古いものを削除
    while (monitorMessages.size() > MAX_MONITOR_MESSAGES)
    {
        monitorMessages.pop_front();
    }
    
    // テキストエディタに表示
    juce::String displayText;
    for (const auto& msg : monitorMessages)
    {
        displayText += msg + "\n";
    }
    
    monitorDisplay.setText(displayText, false);
    monitorDisplay.moveCaretToEnd();
}
