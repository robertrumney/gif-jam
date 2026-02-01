#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GifSyncVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit GifSyncVSTAudioProcessorEditor(GifSyncVSTAudioProcessor&);
    ~GifSyncVSTAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    struct GifFrame
    {
        juce::Image image;
        double endTimeSec = 0.0;
    };

    void timerCallback() override;

    void showContextMenu();
    void loadGifFromFile(const juce::File& file);

    const GifFrame* getFrameForSyncedTime(double syncedTime01) const noexcept;

    GifSyncVSTAudioProcessor& processor;

    juce::File lastGifFile;
    juce::String statusText;

    juce::Array<GifFrame> frames;
    double totalGifSec = 0.0;

    double freeRunPhaseSec = 0.0;
    double lastTimerSec = 0.0;

    double syncBars = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GifSyncVSTAudioProcessorEditor)
};
