#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "InputManager.h"
#include "ThemeColours.h"

class SettingsComponent : public juce::Component, public juce::Timer
{
public:
    SettingsComponent(juce::AudioDeviceManager& dm, InputManager& im)
        : inputManager(im), deviceManager(dm)
    {
        // Apply Dark Theme
        darkLAF.setColourScheme(juce::LookAndFeel_V4::getMidnightColourScheme());
        
        // 1. Audio Device Selector
        audioSelector.reset(new juce::AudioDeviceSelectorComponent(dm,
                                                                   0, 8,   // min/max input（8chまで対応）
                                                                   0, 2,   // min/max output
                                                                   false, false,
                                                                   true, true));
        audioSelector->setLookAndFeel(&darkLAF);
        addAndMakeVisible(audioSelector.get());

        // 2. チャンネル情報ラベル
        channelInfoLabel.setColour(juce::Label::textColourId, ThemeColours::Silver);
        channelInfoLabel.setFont(juce::FontOptions(14.0f));
        addAndMakeVisible(channelInfoLabel);
        
        // 3. ステレオ/モノ切替
        stereoModeButton.setButtonText("Stereo Linked");
        stereoModeButton.setClickingTogglesState(true);
        stereoModeButton.setToggleState(true, juce::dontSendNotification);
        stereoModeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        stereoModeButton.setColour(juce::TextButton::buttonOnColourId, ThemeColours::NeonCyan);
        stereoModeButton.onClick = [this]() {
            bool isStereo = stereoModeButton.getToggleState();
            stereoModeButton.setButtonText(isStereo ? "Stereo Linked" : "Mono (L/R Separate)");
            inputManager.setStereoLinked(isStereo);
        };
        addAndMakeVisible(stereoModeButton);
        
        // 4. キャリブレーションボタン
        calibrateButton.setButtonText("Calibrate");
        calibrateButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        calibrateButton.onClick = [this]() {
            if (inputManager.isCalibrating())
            {
                inputManager.stopCalibration();
                calibrateButton.setButtonText("Calibrate");
            }
            else
            {
                inputManager.startCalibration();
                calibrateButton.setButtonText("Stop (2s)");
            }
        };
        addAndMakeVisible(calibrateButton);
        
        // 5. キャリブレーションON/OFF
        useCalibrationButton.setButtonText("Use Calibration");
        useCalibrationButton.setClickingTogglesState(true);
        useCalibrationButton.setToggleState(true, juce::dontSendNotification);
        useCalibrationButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        useCalibrationButton.setColour(juce::TextButton::buttonOnColourId, ThemeColours::PlayingGreen);
        useCalibrationButton.onClick = [this]() {
            bool useCalib = useCalibrationButton.getToggleState();
            inputManager.setCalibrationEnabled(useCalib);
            useCalibrationButton.setButtonText(useCalib ? "Use Calibration" : "Calibration OFF");
        };
        addAndMakeVisible(useCalibrationButton);
        
        // 6. Threshold Slider
        thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        thresholdSlider.setRange(0.001, 1.0, 0.001); 
        thresholdSlider.setSkewFactorFromMidPoint(0.1);
        thresholdSlider.setColour(juce::Slider::trackColourId, juce::Colours::grey);
        thresholdSlider.setColour(juce::Slider::thumbColourId, ThemeColours::NeonCyan);
        
        // Initial config
        float currentThresh = im.getConfig().userThreshold;
        thresholdSlider.setValue(currentThresh, juce::dontSendNotification);
        
        thresholdSlider.onValueChange = [this]() {
            auto conf = inputManager.getConfig();
            conf.userThreshold = (float)thresholdSlider.getValue();
            inputManager.setConfig(conf);
            // 全チャンネルに閾値を設定
            inputManager.getChannelManager().setGlobalThreshold((float)thresholdSlider.getValue());
        };
        addAndMakeVisible(thresholdSlider);

        // Label
        threshLabel.setText("Trigger Threshold", juce::dontSendNotification);
        threshLabel.attachToComponent(&thresholdSlider, true);
        threshLabel.setJustificationType(juce::Justification::right);
        threshLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(threshLabel);

        startTimerHz(60); // Animation rate
        setSize(600, 600);
    }
    
    ~SettingsComponent() override
    {
         audioSelector->setLookAndFeel(nullptr); // clear before destruction
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        if (audioSelector)
            audioSelector->setBounds(area.removeFromTop(280));

        area.removeFromTop(15);

        // チャンネル情報行
        auto infoRow = area.removeFromTop(30);
        channelInfoLabel.setBounds(infoRow.removeFromLeft(200));
        stereoModeButton.setBounds(infoRow.removeFromLeft(150).reduced(5, 0));
        
        area.removeFromTop(10);
        
        // キャリブレーション行
        auto calibRow = area.removeFromTop(35);
        useCalibrationButton.setBounds(calibRow.removeFromLeft(150).reduced(5, 0));
        calibrateButton.setBounds(calibRow.removeFromLeft(100).reduced(5, 0));

        area.removeFromTop(10);

        // Slider
        auto sliderArea = area.removeFromTop(40);
        thresholdSlider.setBounds(sliderArea.reduced(20, 0).removeFromRight(350));
        
        // Meter Area is drawn in paint below the slider
    }
    
    void timerCallback() override
    {
        // チャンネル情報を更新
        updateChannelInfo();
        
        // キャリブレーション状態を更新
        if (inputManager.isCalibrating())
        {
            calibrateButton.setButtonText("Measuring...");
        }
        else if (calibrateButton.getButtonText() == "Measuring...")
        {
            calibrateButton.setButtonText("Calibrate");
        }
        
        repaint();
    }
    
    void updateChannelInfo()
    {
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            int numInputChannels = device->getActiveInputChannels().countNumberOfSetBits();
            channelInfoLabel.setText("Input Channels: " + juce::String(numInputChannels), 
                                     juce::dontSendNotification);
            
            // InputManagerにチャンネル数を設定
            if (inputManager.getNumChannels() != numInputChannels)
            {
                inputManager.setNumChannels(numInputChannels);
            }
        }
        else
        {
            channelInfoLabel.setText("Input Channels: --", juce::dontSendNotification);
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        // Dark Background for the whole panel
        g.fillAll(juce::Colour(0xff151515)); 

        // Draw Meter below slider
        juce::Rectangle<float> meterRect(150, 460, 350, 24);
        
        // 1. Meter Channel Background (Deep indented look)
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRect(meterRect);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRect(meterRect, 1.0f);
        
        // Get Levels
        float currentLevel = inputManager.getCurrentLevel();
        float threshold = (float)thresholdSlider.getValue();
        
        // 2. Calculate Meter Width
        float normalizedLevel = juce::jlimit(0.0f, 1.0f, currentLevel);
        float meterWidth = meterRect.getWidth() * normalizedLevel;
        
        juce::Rectangle<float> barRect = meterRect.withWidth(meterWidth);
        
        // 3. Futuristic Gradient Fill
        if (meterWidth > 0)
        {
            bool isTriggering = currentLevel > threshold;
            
            juce::ColourGradient gradient(ThemeColours::NeonCyan, meterRect.getX(), meterRect.getY(),
                                          isTriggering ? ThemeColours::RecordingRed : ThemeColours::NeonMagenta, 
                                          meterRect.getRight(), meterRect.getY(), false);
            
            // Add intermediate color for richness
            gradient.addColour(0.5, ThemeColours::ElectricBlue);
            
            g.setGradientFill(gradient);
            g.fillRect(barRect);
        }

        // 4. Grid Lines (Digital Look)
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        for (int i = 1; i < 20; ++i)
        {
            float x = meterRect.getX() + meterRect.getWidth() * (i / 20.0f);
            g.drawLine(x, meterRect.getY(), x, meterRect.getBottom(), 1.0f);
        }

        // 5. Draw Threshold Line (Bright & Clear)
        float threshX = meterRect.getX() + meterRect.getWidth() * threshold;
        g.setColour(juce::Colours::white);
        g.drawLine(threshX, meterRect.getY() - 4, threshX, meterRect.getBottom() + 4, 3.0f);
        
        // 6. キャリブレーション状態表示
        if (inputManager.isCalibrating())
        {
            g.setColour(ThemeColours::ElectricBlue.withAlpha(0.8f));
            g.drawRect(meterRect.expanded(3), 2.0f);
            g.setFont(juce::Font(juce::FontOptions("Arial", 12.0f, juce::Font::plain)));
            g.drawText("CALIBRATING...", meterRect.getRight() + 10, meterRect.getY(), 120, 24, juce::Justification::left);
        }
        
        // モノラルモード表示
        if (!inputManager.isStereoLinked())
        {
            g.setColour(ThemeColours::NeonMagenta);
            g.setFont(juce::Font(juce::FontOptions("Arial", 10.0f, juce::Font::plain)));
            g.drawText("+3dB Boost", meterRect.getX(), meterRect.getBottom() + 5, 80, 16, juce::Justification::left);
        }
        
        // 7. Peak Hold / Trigger Indicator
        bool isTriggering = currentLevel > threshold;
        if (isTriggering && !inputManager.isCalibrating())
        {
            // Flash Effect Frame
            g.setColour(ThemeColours::RecordingRed.withAlpha(0.8f));
            g.drawRect(meterRect.expanded(3), 2.0f);
            
            // Text Warning
            g.setColour(ThemeColours::RecordingRed);
            g.setFont(juce::Font(juce::FontOptions("Arial", 14.0f, juce::Font::bold)));
            g.drawText("TRIGGER", meterRect.getRight() + 10, meterRect.getY(), 80, 24, juce::Justification::left);
        }
        
        // 8. キャリブレーション無効時の表示
        if (!inputManager.isCalibrationEnabled())
        {
            g.setColour(juce::Colours::orange);
            g.setFont(juce::Font(juce::FontOptions("Arial", 10.0f, juce::Font::plain)));
            g.drawText("Gate Open (No Calibration)", meterRect.getX(), meterRect.getY() - 18, 200, 16, juce::Justification::left);
        }
    }

private:
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSelector;
    juce::Slider thresholdSlider;
    juce::Label threshLabel;
    juce::Label channelInfoLabel;
    juce::TextButton stereoModeButton;
    juce::TextButton calibrateButton;
    juce::TextButton useCalibrationButton;
    
    juce::LookAndFeel_V4 darkLAF;
    
    InputManager& inputManager;
    juce::AudioDeviceManager& deviceManager;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

