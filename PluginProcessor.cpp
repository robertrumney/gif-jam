#include "PluginProcessor.h"
#include "PluginEditor.h"

GifSyncVSTAudioProcessor::GifSyncVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

GifSyncVSTAudioProcessor::~GifSyncVSTAudioProcessor() {}

const juce::String GifSyncVSTAudioProcessor::getName() const { return JucePlugin_Name; }

bool GifSyncVSTAudioProcessor::acceptsMidi() const { return false; }
bool GifSyncVSTAudioProcessor::producesMidi() const { return false; }
bool GifSyncVSTAudioProcessor::isMidiEffect() const { return false; }
double GifSyncVSTAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GifSyncVSTAudioProcessor::getNumPrograms() { return 1; }
int GifSyncVSTAudioProcessor::getCurrentProgram() { return 0; }
void GifSyncVSTAudioProcessor::setCurrentProgram(int) {}
const juce::String GifSyncVSTAudioProcessor::getProgramName(int) { return {}; }
void GifSyncVSTAudioProcessor::changeProgramName(int, const juce::String&) {}

void GifSyncVSTAudioProcessor::prepareToPlay(double, int) {}
void GifSyncVSTAudioProcessor::releaseResources() {}

#if ! JucePlugin_PreferredChannelConfigurations
bool GifSyncVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void GifSyncVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (auto* ph = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (ph->getCurrentPosition(info))
        {
            if (info.bpm > 0.0)
                bpm.store(info.bpm, std::memory_order_relaxed);

            isPlaying.store(info.isPlaying, std::memory_order_relaxed);

            if (info.ppqPosition >= 0.0)
                ppqPosition.store(info.ppqPosition, std::memory_order_relaxed);
        }
    }
}

bool GifSyncVSTAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* GifSyncVSTAudioProcessor::createEditor()
{
    return new GifSyncVSTAudioProcessorEditor(*this);
}

void GifSyncVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("GifSyncState");
    xml.setAttribute("bpm", bpm.load(std::memory_order_relaxed));
    xml.setAttribute("syncBars", syncBars.load(std::memory_order_relaxed));
    xml.setAttribute("gifPath", lastGifPath);
    copyXmlToBinary(xml, destData);
}

void GifSyncVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("GifSyncState"))
        {
            const auto v = xml->getDoubleAttribute("bpm", 120.0);
            if (v > 0.0)
                bpm.store(v, std::memory_order_relaxed);

            syncBars.store(xml->getDoubleAttribute("syncBars", 1.0), std::memory_order_relaxed);
            lastGifPath = xml->getStringAttribute("gifPath", {});
            return;
        }
    }

    juce::MemoryInputStream mi(data, (size_t) sizeInBytes, false);
    auto v = mi.readDouble();
    if (v > 0.0)
        bpm.store(v, std::memory_order_relaxed);
}

juce::String GifSyncVSTAudioProcessor::getLastGifPath() const
{
    return lastGifPath;
}

void GifSyncVSTAudioProcessor::setLastGifPath(const juce::String& path)
{
    lastGifPath = path;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GifSyncVSTAudioProcessor();
}
