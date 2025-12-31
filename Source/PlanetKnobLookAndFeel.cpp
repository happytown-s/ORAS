/*
  ==============================================================================

    PlanetKnobLookAndFeel.cpp
    Created: 31 Dec 2025
    Author:  mt sh

  ==============================================================================
*/

#include "PlanetKnobLookAndFeel.h"

void PlanetKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                                             juce::Slider& slider)
{
    // Identify style from component properties
    juce::String styleName = slider.getProperties().getWithDefault("PlanetStyle", "IceBlue");
    
    juce::Colour mainColour;
    
    if (styleName == "MagmaRed") {
        mainColour = ThemeColours::NeonMagenta;
    } else if (styleName == "GasGiant") {
        mainColour = ThemeColours::ElectricBlue;
    } else if (styleName == "BlackHole") {
        mainColour = ThemeColours::Silver;
    } else { // IceBlue (Default) -> NeonCyan
        mainColour = ThemeColours::NeonCyan;
    }

    // --- Layout ---
    // Knob is slightly smaller than the full bounds to make room for the arc
    float diameter = juce::jmin(width, height) * 0.7f;
    float radius = diameter * 0.5f;
    float centreX = x + width * 0.5f;
    float centreY = y + height * 0.5f;
    float rx = centreX - radius;
    float ry = centreY - radius;
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // --- 1. Background Arc (Track) ---
    juce::Path trackPath;
    trackPath.addArc(centreX - radius * 1.2f, centreY - radius * 1.2f, 
                     radius * 2.4f, radius * 2.4f, 
                     rotaryStartAngle, rotaryEndAngle, true);
    
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    g.strokePath(trackPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // --- 2. Value Arc (Glowing) ---
    if (slider.isEnabled())
    {
        juce::Path valuePath;
        valuePath.addArc(centreX - radius * 1.2f, centreY - radius * 1.2f, 
                         radius * 2.4f, radius * 2.4f, 
                         rotaryStartAngle, angle, true);
                         
        // Glow effect: Draw thicker, transparent lines first
        g.setColour(mainColour.withAlpha(0.15f));
        g.strokePath(valuePath, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour(mainColour.withAlpha(0.4f));
        g.strokePath(valuePath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Core line
        g.setColour(mainColour);
        g.strokePath(valuePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- 3. Knob Body (Dark, Modern) ---
    juce::Colour bodyBase = ThemeColours::Background.brighter(0.05f); // Very dark grey
    juce::Colour bodyHighlight = bodyBase.brighter(0.1f);
    juce::Colour bodyShadow = bodyBase.darker(0.8f);

    juce::ColourGradient knobGradient(bodyHighlight, centreX, ry,
                                      bodyShadow, centreX, ry + diameter, false);
    
    // Fill Knob
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, diameter, diameter);
    
    // Knob Rim/Bevel
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawEllipse(rx, ry, diameter, diameter, 1.0f);
    
    // --- 4. Indicator Line (Pointer) ---
    juce::Path p;
    float pointerLength = radius * 0.7f;
    float pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius + 5.0f, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    // Pointer Shadow/Glow
    g.setColour(mainColour.withAlpha(0.3f));
    g.fillPath(p);
    
    // Pointer Core
    g.setColour(mainColour.brighter(0.2f));
    juce::Path pCore;
    pCore.addRectangle(-1.0f, -radius + 6.0f, 2.0f, pointerLength - 2.0f);
    pCore.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.fillPath(pCore);
}
