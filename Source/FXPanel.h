/*
  ==============================================================================

    FXPanel.h
    Created: 31 Dec 2025
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeColours.h"
#include "PlanetKnobLookAndFeel.h"
#include "LooperAudio.h"
#include "FXSlotButtonLookAndFeel.h"

// =====================================================
// 🎛 FXPanel クラス宣言
// =====================================================
class FXPanel: public juce::Component
{
public:
    enum class EffectType {
        None,
        Filter,
        Compressor,
        Delay,
        Reverb,
        BeatRepeat
    };

    struct EffectSlot {
        EffectType type = EffectType::None;
        bool isBypassed = false;
    };

    FXPanel(LooperAudio& looperRef);
    ~FXPanel() override;

    void setTargetTrackId(int trackId);
    
    // トラック選択時のコールバック
    std::function<void(int)> onTrackSelected;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;

private:
    LooperAudio& looper;
    int currentTrackId = -1;
    juce::Label titleLabel;

    PlanetKnobLookAndFeel planetLnF;
    FXSlotButtonLookAndFeel slotLnF;

    // Slots
    EffectSlot slots[4];
    int selectedSlotIndex = 0;
    juce::TextButton slotButtons[4];  // エフェクトスロットボタン
    
    // トラック選択ボタン（8トラック）
    juce::TextButton trackButtons[8];
    
    // Sliders (All exist, visibility toggled)
    // Filter
    juce::Slider filterSlider;
    juce::Label filterLabel;
    juce::Slider filterResSlider; // Resonance
    juce::Label filterResLabel;
    juce::TextButton filterTypeButton { "LPF" }; // Toggle LPF/HPF
    
    // Compressor
    juce::Slider compThreshSlider;  // Threshold
    juce::Label compThreshLabel;
    juce::Slider compRatioSlider;   // Ratio
    juce::Label compRatioLabel;
    
    // Delay
    juce::Slider delaySlider; // Time
    juce::Label delayLabel;
    juce::Slider delayFeedbackSlider;
    juce::Label delayFeedbackLabel;
    juce::Slider delayMixSlider;
    juce::Label delayMixLabel;
    
    // Reverb
    juce::Slider reverbSlider; // Mix
    juce::Label reverbLabel;
    juce::Slider reverbDecaySlider; // RoomSize
    juce::Label reverbDecayLabel;
    
    // Beat Repeat
    juce::Slider repeatDivSlider;
    juce::Label repeatDivLabel;
    juce::Slider repeatThreshSlider;
    juce::Label repeatThreshLabel;
    juce::TextButton repeatActiveButton { "REPEAT OFF" };

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name, const juce::String& style);
    void showEffectMenu(int slotIndex);
    void updateSliderVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPanel)
};
