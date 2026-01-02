/*
  ==============================================================================

    FilterSpectrumVisualizer.h
    Created: 02 Jan 2026
    Author:  mt sh

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeColours.h"

class FilterSpectrumVisualizer : public juce::Component, public juce::Timer
{
public:
    FilterSpectrumVisualizer()
        : forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setOpaque(false);
        startTimerHz(30); // 30 FPS for smooth updates
    }
    
    // Updates internal FFT data from buffer (called from UI thread)
    void pushBuffer(const juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() > 0)
        {
            auto* channelData = buffer.getReadPointer(0);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                pushSampleIntoFifo(channelData[i]);
        }
    }
    
    // Set Filter Parameters for Curve Calculation
    void setFilterParameters(float cutoff, float q, int type)
    {
        filterCutoff = cutoff;
        filterQ = q;
        filterType = type; // 0: LPF, 1: HPF (matches LooperAudio logic)
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ThemeColours::MetalGray.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // 1. Draw Spectrum
        drawSpectrum(g, bounds);
        
        // 2. Draw Filter Curve
        drawFilterCurve(g, bounds);
    }
    
    void timerCallback() override
    {
        if (nextFFTBlockReady)
        {
            drawNextFrameOfSpectrum();
            nextFFTBlockReady = false;
            repaint();
        }
    }

private:
    static constexpr int fftOrder = 10; // 1024 points
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int scopeSize = 256;
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    
    float fifo[fftSize];
    float fftData[fftSize * 2];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float scopeData[scopeSize];
    
    // Filter Params
    float filterCutoff = 20000.0f;
    float filterQ = 0.707f;
    int filterType = 0; // 0: LPF, 1: HPF
    
    void pushSampleIntoFifo(float sample) noexcept
    {
        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                juce::zeromem(fftData, sizeof(fftData));
                std::memcpy(fftData, fifo, sizeof(fifo));
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }
    
    void drawNextFrameOfSpectrum()
    {
        window.multiplyWithWindowingTable(fftData, fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);
        
        auto mindB = -100.0f;
        auto maxdB = 0.0f;
        const float decayRate = 0.8f; 
        
        for (int i = 0; i < scopeSize; ++i)
        {
            // Logarithmic mapping logic
            // scopeSize points map to 0 -> Nyquist
            // But we want to map them logarithmically to match visual
            // Simple approach: linear mapping of FFT bins to scope for now, 
            // but we will draw it logarithmically in 'drawSpectrum'
            
            // Actually, usually we store linear bins in scopeData and map x coordinate logarithmically.
            // Let's just process FFT bins into scopeData linearly and handle log X in drawing.
            
            // Map scope index to FFT bin
            // We want 20Hz - 20kHz range roughly
            
            int fftDataIndex = i * (fftSize / 2) / scopeSize;
            
            auto level = juce::jmap(juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize), mindB, maxdB, 0.0f, 1.0f);
            
            level = juce::jlimit(0.0f, 1.0f, level);
            
             if (level > scopeData[i])
                scopeData[i] = level;
            else
                scopeData[i] = scopeData[i] * decayRate + level * (1.0f - decayRate);
        }
    }
    
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        juce::Path p;
        p.startNewSubPath(bounds.getBottomLeft());
        
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;
        const float sr = 48000.0f; // Assuming 48k for viz
        
        for (int i = 0; i < scopeSize; ++i)
        {
            // Calculate frequency of this bin
            float freq = (float)i * (sr * 0.5f) / (float)scopeSize;
            
            // Avoid drawing below minFreq
            if(freq < minFreq) continue;
            
            // Logarithmic X position
            float normX = std::log(freq / minFreq) / std::log(maxFreq / minFreq);
            float x = bounds.getX() + normX * bounds.getWidth();
            
            float level = scopeData[i];
            float y = bounds.getBottom() - level * bounds.getHeight();
            
            p.lineTo(x, y);
        }
        
        p.lineTo(bounds.getBottomRight());
        p.closeSubPath();
        
        juce::ColourGradient grad(ThemeColours::NeonCyan.withAlpha(0.6f), bounds.getBottomLeft(),
                                  ThemeColours::NeonMagenta.withAlpha(0.6f), bounds.getTopRight(), false);
        g.setGradientFill(grad);
        g.fillPath(p);
    }
    
    void drawFilterCurve(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        juce::Path p;
        
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;
        const int numPoints = 200;
        
        // Calculate filter coefficients (Analog Prototype for StateVariable)
        // H(s) for LPF = 1 / (s^2 + s/Q + 1) normalized
        // We use the digital equivalent magnitude response directly.
        
        // Simplified magnitude response calculation for visualization
        // w = 2*pi*f/sr
        // This is an approximation or direct calculation of SVF transfer function
        
        bool started = false;
        
        for(int i=0; i<numPoints; ++i)
        {
            float xNorm = (float)i / (float)(numPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, xNorm);
            
            float mag = getFilterMagnitude(freq);
            
            // Map magnitude (dB) to Y. Range: +10dB to -60dB ?
            // Let's assume height represents 0dB to -40dB roughly, but allow peaking
            // Center (0dB) at 20% from top?
            // User wants "Envelope". Usually 0dB is top, -inf is bottom.
            // With resonance, it goes above 0dB.
            
            float db = juce::Decibels::gainToDecibels(mag);
            
            // Mapping: 0dB -> 30% from top. +12dB -> Top. -60dB -> Bottom.
            float yNorm = 1.0f - juce::jmap(db, -60.0f, 15.0f, 0.0f, 1.0f);
            float y = bounds.getY() + yNorm * bounds.getHeight();
            
            float x = bounds.getX() + xNorm * bounds.getWidth();
            
            if(!started) { p.startNewSubPath(x, y); started = true; }
            else { p.lineTo(x, y); }
        }
        
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.strokePath(p, juce::PathStrokeType(2.0f));
    }
    
    // Calculate discrete time frequency response of SVF
    float getFilterMagnitude(float freq)
    {
        double sr = 48000.0; // Fixed for viz
        double w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
        
        // TPT SVF Coefficients preparation
        // g = tan(w_c * T / 2) -> here we use pre-warped cutoff
        // but for viz, direct Laplace |H(jw)| is often "good enough" and smoother?
        // Let's use the actual digital transfer function for accuracy.
        
        double wc = 2.0 * juce::MathConstants<double>::pi * filterCutoff / sr;
        double g = std::tan(wc * 0.5);
        double R = 1.0 / (2.0 * filterQ); // Damping factor used in some derivations, or 1/Q used below?
        // JUCE impl uses R = 1 / (2Q)
        
         // Transfer functions H(z) derived from TPT structure:
        // LPF: g^2 / (1 + 2Rg + g^2) * (1 + z^-1)^2 / D
        // HPF: 1   / (1 + 2Rg + g^2) * (1 - z^-1)^2 / D
        // D = 1 - 2(1 - g^2)/(1 + 2Rg + g^2) z^-1 + ... complex.
        
        // Easier way: Evaluate Z-domain transfer function at z = e^(jw)
        std::complex<double> z = std::exp(std::complex<double>(0, w));
        
        // Coefficient calculation (from standard SVF TPT)
        // h = 1 / (1 + 2*R*g + g*g)
        double R_val = 1.0 / (2.0 * filterQ);
        double h_factor = 1.0 / (1.0 + 2.0 * R_val * g + g * g);
        
        std::complex<double> transferH;
        
        if (filterType == 0) // LPF
        {
            // H_LP(z) = (g^2 * h) * (1 + z^-1)^2 / (1 - 2*z^-1*( (1-g^2 - 2Rg) / (same denom? wait) ) )
            // Actually, let's use the nice property:
            // H_LP(s) = w_c^2 / (s^2 + (w_c/Q)s + w_c^2)
            // |H_LP(jw)| = 1 / sqrt( (1 - (w/w_c)^2)^2 + (w/(Q*w_c))^2 ) is for analog.
            // With warping: w_analog = tan(w_digital/2).
            // This is perfect for TPT which matches analog curve via bilinear transform.
            
            double wa = std::tan(w / 2.0); // Pre-warped freq
            double wca = std::tan(wc / 2.0); // Cutoff target
            
            // Normalize to cutoff
            double Omega = wa / wca;
            // |H(jOmega)| = 1 / sqrt( (1 - Omega^2)^2 + (Omega/Q)^2 )
            
            double denom = std::sqrt(std::pow(1.0 - Omega * Omega, 2.0) + std::pow(Omega / filterQ, 2.0));
            return (float)(1.0 / denom);
        }
        else // HPF
        {
            // |H_HP(jOmega)| = Omega^2 / sqrt( (1 - Omega^2)^2 + (Omega/Q)^2 )
             double wa = std::tan(w / 2.0);
            double wca = std::tan(wc / 2.0);
            double Omega = wa / wca;
            
            double num = Omega * Omega;
            double denom = std::sqrt(std::pow(1.0 - Omega * Omega, 2.0) + std::pow(Omega / filterQ, 2.0));
            return (float)(num / denom);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterSpectrumVisualizer)
};
