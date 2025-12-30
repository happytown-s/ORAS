#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeColours.h"

class CircularVisualizer : public juce::Component, public juce::Timer
{
public:
    CircularVisualizer()
        : forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setOpaque(false); // Make transparent!
        startTimerHz(60);
    }

    void pushBuffer(const juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() > 0)
        {
            auto* channelData = buffer.getReadPointer(0);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                pushSampleIntoFifo(channelData[i]);
        }
    }

    // 波形データを追加（履歴として管理）
    // trackLengthSamples: このトラックの録音長
    // masterLengthSamples: 現在のマスターのループ長（1周期の長さ）
    // recordStartGlobal: 録音開始時のグローバル絶対位置
    // masterStartGlobal: マスターのループ開始時のグローバル絶対位置
    void addWaveform(int trackId, const juce::AudioBuffer<float>& buffer, 
                     int trackLengthSamples, int masterLengthSamples, 
                     int recordStartGlobal = 0, int masterStartGlobal = 0)
    {
        // ⚠️ trackLengthSamplesを使用（buffer.getNumSamples()ではない！）
        // buffer.getNumSamples()はバッファ全体のサイズで、実際の録音長ではない
        const int numSamples = trackLengthSamples;
        if (numSamples == 0 || masterLengthSamples == 0) return;

        const auto* data = buffer.getReadPointer(0);
        const int points = 1024; 
        
        // マスターループに対して、波形が円の何割を占めるか
        // 通常、最初のトラック（マスター）は 1.0 になるべき
        double loopRatio = (double)trackLengthSamples / (double)masterLengthSamples;
        
        // マスターとほぼ同じ長さなら、誤差を許容して 1.0 に丸める
        if (loopRatio > 0.95 && loopRatio < 1.05) loopRatio = 1.0;

        // 開始位置のオフセット計算
        // recordStartGlobal と masterStartGlobal が同じなら startAngleRatio = 0
        long offsetFromMasterStart = (long)recordStartGlobal - (long)masterStartGlobal;
        double startAngleRatio = 0.0;
        
        if (masterLengthSamples > 0 && offsetFromMasterStart > 0)
        {
            int relativeStartSample = (int)(offsetFromMasterStart % masterLengthSamples);
            startAngleRatio = (double)relativeStartSample / (double)masterLengthSamples;
        }

        const int samplesPerPoint = numSamples / points;
        if (samplesPerPoint < 1) return;

        juce::Path newPath;
        const float maxAmpWidth = 0.3f; // Increased for more visible waves

        for (int i = 0; i <= points; ++i)
        {
            float rms = 0.0f;
            int startSample = i * samplesPerPoint;
            for (int j = 0; j < samplesPerPoint; ++j)
            {
                if (startSample + j < numSamples)
                    rms += std::abs(data[startSample + j]);
            }
            rms /= (float)samplesPerPoint;
            rms = std::pow(rms, 0.6f);

            // 精度向上のため、サンプル数ベースで進行度を計算
            // `i / points` だと割り切りれない場合に末尾が引き伸ばされてズレる要因になる
            double currentSamplePos = (double)(i * samplesPerPoint);
            double progressRaw = currentSamplePos / (double)trackLengthSamples;
            
            // 角度計算: 
            // 開始角(startAngleRatio) + 進行角(progress * loopRatio)
            double currentAngleRatio = startAngleRatio + (progressRaw * loopRatio);

            // 3時(0度)開始 + レイテンシー補正
            // レイテンシー補正: 出力バッファ分(約512サンプル)だけ波形を時計回りにずらす
            double latencyDelay = 1024.0 / (double)trackLengthSamples;
            float angle = (float)(juce::MathConstants<double>::twoPi * (currentAngleRatio + latencyDelay));
            
            float rInner = 1.0f - (rms * maxAmpWidth);
            float xIn = rInner * std::cos(angle);
            float yIn = rInner * std::sin(angle);
            
             if (i == 0) newPath.startNewSubPath(xIn, yIn);
             else        newPath.lineTo(xIn, yIn);
        }

        // 外側の点を逆順に追加
        for (int i = points; i >= 0; --i)
        {
            float rms = 0.0f;
            int startSample = i * samplesPerPoint;
            for (int j = 0; j < samplesPerPoint; ++j)
            {
                if (startSample + j < numSamples)
                    rms += std::abs(data[startSample + j]);
            }
            rms /= (float)samplesPerPoint;
            rms = std::pow(rms, 0.6f);

            double currentSamplePos = (double)(i * samplesPerPoint);
            double progressRaw = currentSamplePos / (double)trackLengthSamples;
            
            double currentAngleRatio = startAngleRatio + (progressRaw * loopRatio);
            
            double latencyDelay = 1024.0 / (double)trackLengthSamples;
            float angle = (float)(juce::MathConstants<double>::twoPi * (currentAngleRatio + latencyDelay));
            
            float rOuter = 1.0f + (rms * maxAmpWidth);
            float xOut = rOuter * std::cos(angle);
            float yOut = rOuter * std::sin(angle);
            
            newPath.lineTo(xOut, yOut);
        }
        
        newPath.closeSubPath();

        // 履歴に追加
        WaveformPath wp;
        wp.path = newPath;
        wp.trackId = trackId;
        
        // 色の決定 (とりあえず簡易的なプリセット)
        switch ((trackId - 1) % 4) {
            case 0: wp.colour = ThemeColours::NeonCyan; break;
            case 1: wp.colour = ThemeColours::NeonMagenta; break;
            case 2: wp.colour = juce::Colours::orange; break;
            case 3: wp.colour = juce::Colours::lime; break;
            default: wp.colour = ThemeColours::NeonCyan; break;
        }

        waveformPaths.insert(waveformPaths.begin(), wp);
        if (waveformPaths.size() > 5) waveformPaths.resize(5);
        
        repaint();
    }

    void setPlayHeadPosition(float normalizedPos)
    {
        currentPlayHeadPos = normalizedPos;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;

        // --- Visualizer Elements (Overlay only) ---
        
        // Background circle
        g.setColour(ThemeColours::MetalGray.withAlpha(0.1f));
        g.fillEllipse(bounds.withSizeKeepingCentre(radius * 2.0f, radius * 2.0f));
        g.setColour(ThemeColours::MetalGray.withAlpha(0.3f));
        g.drawEllipse(bounds.withSizeKeepingCentre(radius * 2.1f, radius * 2.1f), 1.0f);

        // --- Draw Concentric Waveforms with Glow ---
        for (int i = 0; i < (int)waveformPaths.size(); ++i)
        {
            const auto& wp = waveformPaths[i];
            
            float scaleLayer = 1.0f - (float)i * 0.18f;
            if (scaleLayer <= 0.1f) continue;
            
            float finalScale = radius * scaleLayer;
            float baseAlpha = 0.9f - (float)i * 0.12f; 
            
            auto transform = juce::AffineTransform::scale(finalScale, finalScale)
                                                   .translated(centre.x, centre.y);
            
            juce::Path p = wp.path;
            p.applyTransform(transform);
            
            // Outer glow layers (luminous effect)
            for (int glow = 3; glow >= 1; --glow)
            {
                float glowAlpha = baseAlpha * 0.15f / (float)glow;
                g.setColour(wp.colour.withAlpha(juce::jlimit(0.05f, 0.4f, glowAlpha)));
                g.strokePath(p, juce::PathStrokeType(glow * 3.0f));
            }
            
            // Main fill with gradient-like brightness
            g.setColour(wp.colour.withAlpha(juce::jlimit(0.3f, 0.85f, baseAlpha)));
            g.fillPath(p);
            
            // Inner bright stroke (core light)
            g.setColour(wp.colour.brighter(0.4f).withAlpha(juce::jlimit(0.4f, 1.0f, baseAlpha + 0.3f)));
            g.strokePath(p, juce::PathStrokeType(1.5f));
            
            // Hot center line (brightest)
            g.setColour(juce::Colours::white.withAlpha(juce::jlimit(0.1f, 0.5f, baseAlpha * 0.6f)));
            g.strokePath(p, juce::PathStrokeType(0.5f));
        }
        
        // --- Draw Playhead ---
        if (currentPlayHeadPos >= 0.0f)
        {
            // 3時(0度)開始
            float angle = currentPlayHeadPos * juce::MathConstants<float>::twoPi;
            
            // プレイヘッドライン (レーダーのように中心から外へ)
            auto innerPos = centre.getPointOnCircumference(radius * 0.1f, angle);
            auto outerPos = centre.getPointOnCircumference(radius * 1.1f, angle);
            
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.0f), innerPos.x, innerPos.y,
                                                   juce::Colours::white.withAlpha(0.8f), outerPos.x, outerPos.y, false));
            g.drawLine(innerPos.x, innerPos.y, outerPos.x, outerPos.y, 2.0f);


            auto headPos = centre.getPointOnCircumference(radius, angle);
            g.setColour(juce::Colours::white);
            g.fillEllipse(headPos.x - 3.0f, headPos.y - 3.0f, 6.0f, 6.0f);
        }


        // Draw spinning accent rings
        float rotation = (float)juce::Time::getMillisecondCounterHiRes() * 0.001f;
        g.setColour(ThemeColours::NeonCyan.withAlpha(0.15f));
        drawRotatingRing(g, centre, radius * 1.05f, rotation, 0.4f);
        g.setColour(ThemeColours::NeonMagenta.withAlpha(0.1f));
        drawRotatingRing(g, centre, radius * 1.1f, -rotation * 0.7f, 0.3f);
        
        // Outer ring
        g.setColour(ThemeColours::NeonCyan.withAlpha(0.4f));
        g.drawEllipse(bounds.withSizeKeepingCentre(radius * 2.0f, radius * 2.0f), 1.5f);
        
        // --- Draw Circular Spectrum (Outer Audio Visualizer) ---
        const float spectrumRadius = radius * 1.2f;
        const float maxBarHeight = radius * 0.25f;
        const int numBars = scopeSize / 2; // Use half the scope data for smoother look
        
        for (int i = 0; i < numBars; ++i)
        {
            // 3時(0度)開始
            float angle = (float)i / (float)numBars * juce::MathConstants<float>::twoPi;
            float level = scopeData[i * 2]; // Sample every other data point
            float barHeight = level * maxBarHeight;
            
            if (barHeight < 1.0f) continue; // Skip very small bars
            
            auto innerPoint = centre.getPointOnCircumference(spectrumRadius, angle);
            auto outerPoint = centre.getPointOnCircumference(spectrumRadius + barHeight, angle);
            
            // Color gradient from cyan to magenta based on position
            float hue = 0.5f + (float)i / (float)numBars * 0.3f; // Cyan to purple range
            auto barColor = juce::Colour::fromHSV(hue, 0.8f, 0.9f, juce::jlimit(0.3f, 0.9f, level + 0.3f));
            
            g.setColour(barColor);
            g.drawLine(innerPoint.x, innerPoint.y, outerPoint.x, outerPoint.y, 2.0f);
        }
    }


    // 全リセット
    void clear()
    {
        waveformPaths.clear();
        currentPlayHeadPos = -1.0f;
        juce::zeromem(scopeData, sizeof(scopeData));
        repaint();
    }

    void timerCallback() override
    {
        repaint(); // Always repaint for animations
        
        if (nextFFTBlockReady)
        {
            drawNextFrameOfSpectrum();
            nextFFTBlockReady = false;
        }
    }

private:
   
    struct WaveformPath
    {
        juce::Path path;
        int trackId;
        juce::Colour colour;
    };
    std::vector<WaveformPath> waveformPaths;
    float currentPlayHeadPos = -1.0f;

    void drawRotatingRing(juce::Graphics& g, juce::Point<float> centre, float radius, float rotation, float arcLength)
    {
        juce::Path ring;
        ring.addCentredArc(centre.x, centre.y, radius, radius, rotation, 0.0f, juce::MathConstants<float>::twoPi * arcLength, true);
        g.strokePath(ring, juce::PathStrokeType(1.5f));
    }
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
        const float decayRate = 0.85f; // Slow decay (higher = slower)

        for (int i = 0; i < scopeSize; ++i)
        {
            auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
            auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize / 2));
            auto newLevel = juce::jmap(juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize), mindB, maxdB, 0.0f, 1.0f);
            newLevel = juce::jlimit(0.0f, 1.0f, newLevel);

            // Apply decay: only decrease slowly, increase immediately
            if (newLevel > scopeData[i])
                scopeData[i] = newLevel;
            else
                scopeData[i] = scopeData[i] * decayRate + newLevel * (1.0f - decayRate);
        }
    }

    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int scopeSize = 256;

    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    float fifo[fftSize];
    float fftData[fftSize * 2];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    float scopeData[scopeSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircularVisualizer)
};
