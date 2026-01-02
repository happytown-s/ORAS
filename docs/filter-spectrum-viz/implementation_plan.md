# Filter Envelope & Spectrum Visualization Implementation Plan

## Goal
Visualize the frequency spectrum of the currently selected track and overlay the frequency response curve of the active filter (Cutoff/Resonance).

## User Review Required
> [!NOTE]
> This feature adds real-time FFT processing for visualization. It is lightweight but adds some CPU load on the UI thread.
> Using `juce::AbstractFifo` for thread-safe data transfer from Audio thread to UI thread.

## Proposed Changes

### [LooperAudio](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/LooperAudio.h)
- **Add Monitoring FIFO**:
    - `juce::AbstractFifo monitorFifo { 4096 }`
    - `std::vector<float> monitorBuffer`
    - `std::atomic<int> monitorTrackId { -1 }`
- **Audio Processing**:
    - In `mixTracksToOutput` loop, if `trackId == monitorTrackId`, push samples to `monitorFifo`.

### [NEW] [FilterSpectrumVisualizer](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/FilterSpectrumVisualizer.h)
- **Component**:
    - `juce::dsp::FFT forwardFFT` (Order 10 or 11 = 1024 or 2048 points)
    - `paint(g)`:
        - Draw frequency grid (20Hz - 20kHz, Logarithmic).
        - Draw FFT spectrum (filled gradient).
        - Draw Filter magnitude response (thick line).
- **Filter Curve Calculation**:
    - Calculate magnitude response $H(f)$ for State Variable Filter (Lowpass/Highpass).
    - Use formula: $|H(e^{j\omega})|$ where $\omega = 2\pi f / f_s$.

### [FXPanel](file:///Users/mtsh/書類/code/JUCE/ORAS/Source/FXPanel.cpp)
- **Integration**:
    - Add `FilterSpectrumVisualizer` member.
    - Place it in the layout (e.g., above or beside the filter sliders).
    - In `timerCallback` (or creating one), pull data from `LooperAudio` and push to `FilterSpectrumVisualizer`.
    - Note: `FXPanel` might need to become a `Timer` to drive the visualization updates.

## Verification Plan
### Manual Verification
- Play a track and verify spectrum movement.
- Move Filter Cutoff/Resonance sliders and verify the white curve updates instantly.
- Switch tracks and verify spectrum changes to the new track source.
