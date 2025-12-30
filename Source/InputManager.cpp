/*
  ==============================================================================

    InputManager.cpp
    Created: 8 Oct 2025 3:22:59am
    Author:  mt sh

  ==============================================================================
*/

#include "InputManager.h"

void InputManager::prepare(double newSampleRate, int bufferSize)
{
	sampleRate = newSampleRate;
	triggered  = false;
	recording = false;
	triggerEvent.reset();
	smoothedEnergy = 0.0f;

	// Prepare ring buffer (2 seconds)
    inputBuffer.prepare(sampleRate, 2);

	DBG("InputManager::prepare sampleRate = " << sampleRate << "bufferSize = " << bufferSize);
    DBG("AudioInputBuffer initialized.");
}

void InputManager::reset()
{
	triggerEvent.reset();
	recording = false;
	triggerEvent.sampleInBlock = -1;
	triggerEvent.absIndex = -1;
    // inputBuffer.clear(); // Optional: clear buffer on reset
	DBG("InputManager::reset()");
}

//==============================================================================
// メイン処理
//==============================================================================

void InputManager::analyze(const juce::AudioBuffer<float>& input)
{
    const int numSamples = input.getNumSamples();
    if (numSamples == 0) return;

    // Use channel 0 for trigger analysis and buffering (Mono support for now)
    const float* readPtr = input.getReadPointer(0);
    
    // 1. Write to Ring Buffer
    inputBuffer.write(readPtr, numSamples);

    // Calculate level for UI
    float maxAmp = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absSamp = std::abs(readPtr[i]);
        if (absSamp > maxAmp) maxAmp = absSamp;
    }
    currentLevel.store(maxAmp); // Update atomic level

	// 2. Process Triggers (2-stage logic)
    // Low threshold = silence threshold (noise floor)
    // High threshold = user threshold
    bool trig = inputBuffer.processTriggers(readPtr, numSamples, 
                                            config.silenceThreshold, 
                                            config.userThreshold);

	// 3. State Update
	if (trig && !triggered)
	{
		triggered = true;
        // Fire trigger! AbsIndex is managed by AudioInputBuffer internally mostly,
        // but we signal capability via the event.
		triggerEvent.fire(0, -1);
		DBG("🔥 Trigger Fired! High threshold exceeded.");
	}
	else if (triggered)
	{
        // 🔄 Auto-Reset logic:
        // We only reset 'triggered' when the input signal drops below silence threshold
        // (i.e. Low Threshold) for a sufficient time.
        // AudioInputBuffer handles this logic via 'inPreRoll' state.
        
        if (!inputBuffer.isInPreRoll())
        {
            triggered = false;
            triggerEvent.reset();
            DBG("🔄 Trigger Re-armed (Silence detected)");
        }
	}
}

//==============================================================================
// エネルギー（RMS）を計算
//==============================================================================

float InputManager::computeEnergy(const juce::AudioBuffer<float>& input)
{
	const int numChannels = input.getNumChannels();
	const int numSamples = input.getNumSamples();
	float total = 0.0f;

	for (int ch = 0; ch < numChannels; ++ch)
	{
		const float* data = input.getReadPointer(ch);
		for (int i = 0; i < numSamples; ++i)
		{
			total += data[i] * data[i];
		}
	}

	const float mean = total / (float)(numChannels * numSamples);
	return std::sqrt(mean);
}

//==============================================================================
// 状態遷移（仮）
//==============================================================================
void InputManager::updateStateMachine()
{
	// 将来的に「録音中→停止判定」などをここに追加
}
//==============================================================================
// Getter / Setter
//==============================================================================
juce::TriggerEvent& InputManager::getTriggerEvent() noexcept
{
	return triggerEvent;

}

void InputManager::setConfig(const SmartRecConfig& newConfig) noexcept
{
	config = newConfig;
}

const SmartRecConfig& InputManager::getConfig() const noexcept
{
	return config;
}
//==============================================================================
// 閾値検知：ブロック内の最大音量を確認
//==============================================================================

bool InputManager::detectTriggerSample(const juce::AudioBuffer<float>& input)
{
	const int numChannels = input.getNumChannels();
	const int numSamples = input.getNumSamples();
	const float threshold = config.userThreshold;

	for(int s = 0; s < numSamples; ++s)
	{
		float frameAmp = 0.0f;
		for ( int ch = 0; ch < numChannels; ++ch)
		{
			frameAmp += std::abs(input.getReadPointer(ch)[s]);
		}

		frameAmp /= (float)numChannels;

		if(frameAmp > threshold)
		{
			triggerEvent.sampleInBlock = s;
			triggerEvent.channel = 0;
			return true;
		}
	}
	return false;
}

