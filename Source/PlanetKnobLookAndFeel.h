/*
  ==============================================================================

    PlanetKnobLookAndFeel.h
    Created: 31 Dec 2025
    Author:  mt sh

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeColours.h"

class PlanetKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum class PlanetStyle {
        IceBlue,    // Filter
        MagmaRed,   // Comp
        GasGiant,   // Reverb
        BlackHole   // Delay
    };

    PlanetKnobLookAndFeel() {}
    
    // Static helper to set style on a slider via properties or simpler mechanism?
    // Since LookAndFeel is usually shared or assigned, we can check Slider name or Property.
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                          juce::Slider& slider) override;
};
