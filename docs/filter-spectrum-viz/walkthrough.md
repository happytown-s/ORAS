# Walkthrough - ORAS (formerly Looper)

## Latest Changes: Filter & Spectrum Visualization

### Overview
A new visualization component has been integrated into the FX Panel. It displays a real-time frequency spectrum of the selected track and overlays the frequency response curve of the active filter.

### Features
- **Real-time Spectrum**: Visualizes the audio content of the specific track being edited.
- **Filter Envelope**: Shows the magnitude response curve based on the current Cutoff, Resonance, and Filter Type (LPF/HPF).
- **Interactive**: The white filter curve updates instantly as you move the filter sliders.
- **Integrated UI**: Located directly within the FX Panel when the Filter slot is selected.

### Files Created/Modified
- `Source/FilterSpectrumVisualizer.h` [NEW]: The visualization component.
- `Source/LooperAudio.h/cpp`: Added monitoring FIFO to transfer audio from the audio thread to the UI thread.
- `Source/FXPanel.h/cpp`: Integrated the visualizer and linked it to filter controls.

### Verification Steps
1.  **Launch the Application**: Open ORAS.
2.  **Load/Record Audio**: Ensure a track has audio playing.
3.  **Select Filter Effect**:
    - Click on an empty slot in the FX Panel.
    - Select "Filter" from the menu.
4.  **Observe Visualization**:
    - A new visualizer panel should appear above the filter knobs.
    - You should see the spectrum of the audio moving.
5.  **Adjust Filter Controls**:
    - Move the **FREQ** slider: The white curve should shift left/right.
    - Move the **RES** slider: The peak of the curve should change.
    - Toggle **LPF/HPF**: The curve shape should flip.
6.  **Verify Sound**: Ensure the visual changes correspond to the audible filter effect.

### Previous Changes (Multi-Channel UI)
- **Settings UI**: Tabbed interface (Device / Trigger)
- **16-Channel Support**: Compact grid layout for channel pairs.
- **Input Monitoring**: Device channel names displayed on cards.

## Status
- [x] Multi-Channel 16ch Support
- [x] Settings UI Refactor
- [x] Filter & Spectrum Visualization
