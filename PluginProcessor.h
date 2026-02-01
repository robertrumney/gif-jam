#pragma once
#include <JuceHeader.h>

class GifSyncVSTAudioProcessor final : public juce::AudioProcessor
{
    public:
        GifSyncVSTAudioProcessor();
        ~GifSyncVSTAudioProcessor() override;
    
        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
    
       #if ! JucePlugin_PreferredChannelConfigurations
        bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
       #endif
    
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override;
    
        const juce::String getName() const override;
    
        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;
    
        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int index) override;
        const juce::String getProgramName(int index) override;
        void changeProgramName(int index, const juce::String& newName) override;
    
        void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;
    
        double getBpm() const noexcept { return bpm.load(std::memory_order_relaxed); }
        double getPpqPosition() const noexcept { return ppqPosition.load(std::memory_order_relaxed); }
        bool getIsPlaying() const noexcept { return isPlaying.load(std::memory_order_relaxed); }
    
        juce::String getLastGifPath() const;
        void setLastGifPath(const juce::String& path);
    
        double getSyncBars() const noexcept { return syncBars.load(std::memory_order_relaxed); }
        void setSyncBars(double bars) noexcept { syncBars.store(bars, std::memory_order_relaxed); }
    
    private:
        std::atomic<double> bpm { 120.0 };
        std::atomic<double> ppqPosition { 0.0 };
        std::atomic<bool>   isPlaying { false };
    
        std::atomic<double> syncBars { 1.0 };
        juce::String lastGifPath;
    
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GifSyncVSTAudioProcessor)
};
