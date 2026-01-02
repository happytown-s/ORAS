/*
  ==============================================================================

    FXSlotButtonLookAndFeel.h
    Created: 1 Jan 2026
    Author:  mt sh
    
    FXスロットボタン用のグラスモーフィズムスタイル

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeColours.h"

// FXスロットボタン用のグラスモーフィズムLookAndFeel
class FXSlotButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FXSlotButtonLookAndFeel() = default;
    
    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton,
                            bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        float cornerRadius = 8.0f;
        
        bool isSelected = button.getToggleState();
        
        // グローカラー（選択状態で変化）
        juce::Colour glowColor = isSelected ? ThemeColours::NeonMagenta : ThemeColours::NeonCyan;
        
        // --- 外側のグロー効果 ---
        if (isSelected || isMouseOverButton)
        {
            for (int i = 3; i >= 1; --i)
            {
                float alpha = isSelected ? 0.2f : 0.1f;
                g.setColour(glowColor.withAlpha(alpha / (float)i));
                g.fillRoundedRectangle(bounds.expanded(i * 2.0f), cornerRadius + i);
            }
        }
        
        // --- フロステッドガラス背景 (透明度を上げてよりガラスらしく) ---
        juce::ColourGradient glassGradient(
            juce::Colour(40, 45, 55).withAlpha(isSelected ? 0.7f : 0.4f), 
            bounds.getCentreX(), bounds.getY(),
            juce::Colour(25, 30, 40).withAlpha(isSelected ? 0.8f : 0.5f), 
            bounds.getCentreX(), bounds.getBottom(),
            false
        );
        g.setGradientFill(glassGradient);
        g.fillRoundedRectangle(bounds, cornerRadius);
        
        // --- 上部ハイライト（ガラス反射効果・より鮮明に） ---
        juce::Path highlight;
        highlight.addRoundedRectangle(bounds.getX() + 2, bounds.getY() + 2, 
                                     bounds.getWidth() - 4, bounds.getHeight() * 0.4f,
                                     cornerRadius - 2, cornerRadius - 2, true, true, false, false);
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillPath(highlight);
        
        // --- 押下時の内側シャドウ ---
        if (isButtonDown)
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.reduced(1.0f), cornerRadius - 1);
        }
        
        // --- ボーダー ---
        float borderAlpha = isSelected ? 0.7f : (isMouseOverButton ? 0.4f : 0.2f);
        g.setColour(glowColor.withAlpha(borderAlpha));
        g.drawRoundedRectangle(bounds, cornerRadius, isSelected ? 2.0f : 1.0f);
        
        // --- 内側の微細なアクセントボーダー ---
        g.setColour(juce::Colours::white.withAlpha(isMouseOverButton ? 0.15f : 0.08f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), cornerRadius - 1, 0.5f);
    }
    
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool isMouseOver, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        bool isSelected = button.getToggleState();
        
        // テキストカラー
        juce::Colour textColor = isSelected ? juce::Colours::white : ThemeColours::Silver;
        if (isMouseOver) textColor = textColor.brighter(0.2f);
        
        g.setColour(textColor);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        
        // テキスト描画（アイコンを追加することも可能）
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);
    }
};
