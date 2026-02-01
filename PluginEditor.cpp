#include "PluginEditor.h"
#include <cmath>

#include "stb_image.h"

static double nowSeconds()
{
    return juce::Time::getMillisecondCounterHiRes() * 0.001;
}

static double cycleBeatsFromBars(double bars)
{
    return juce::jmax(0.0001, bars * 4.0);
}

static double cycleSecondsFromBars(double bpm, double bars)
{
    bpm = juce::jmax(1.0, bpm);
    const double secPerBeat = 60.0 / bpm;
    return secPerBeat * cycleBeatsFromBars(bars);
}

GifSyncVSTAudioProcessorEditor::GifSyncVSTAudioProcessorEditor(GifSyncVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setOpaque(true);
    setResizable(false, false);
    setSize(420, 280);

    syncBars = processor.getSyncBars();

    auto savedPath = processor.getLastGifPath();
    if (savedPath.isNotEmpty())
    {
        juce::File f(savedPath);
        if (f.existsAsFile())
            loadGifFromFile(f);
        else
            statusText = "Saved GIF missing. Right-click to load a GIF";
    }
    else
    {
        statusText = "Right-click to load a GIF";
    }

    lastTimerSec = nowSeconds();
    startTimerHz(60);
}

GifSyncVSTAudioProcessorEditor::~GifSyncVSTAudioProcessorEditor() {}

void GifSyncVSTAudioProcessorEditor::resized() {}

const GifSyncVSTAudioProcessorEditor::GifFrame* GifSyncVSTAudioProcessorEditor::getFrameForSyncedTime(double syncedTime01) const noexcept
{
    if (frames.isEmpty() || totalGifSec <= 0.0)
        return nullptr;

    auto t = juce::jlimit(0.0, 1.0, syncedTime01) * totalGifSec;

    for (int i = 0; i < frames.size(); ++i)
        if (t <= frames.getReference(i).endTimeSec)
            return &frames.getReference(i);

    return &frames.getReference(frames.size() - 1);
}

void GifSyncVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (frames.isEmpty())
    {
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(16.0f);
        g.drawFittedText(statusText, getLocalBounds().reduced(12), juce::Justification::centred, 4);
        return;
    }

    const double bpm = juce::jmax(1.0, processor.getBpm());
    const double cycleBeats = cycleBeatsFromBars(syncBars);
    const double cycleSec = cycleSecondsFromBars(bpm, syncBars);

    double synced01 = 0.0;

    if (processor.getIsPlaying())
    {
        const double ppq = processor.getPpqPosition();
        const double phaseBeats = std::fmod(ppq, cycleBeats);
        const double wrapped = (phaseBeats < 0.0 ? phaseBeats + cycleBeats : phaseBeats);
        synced01 = wrapped / cycleBeats;
    }
    else
    {
        synced01 = std::fmod(freeRunPhaseSec / cycleSec, 1.0);
        if (synced01 < 0.0) synced01 += 1.0;
    }

    auto* f = getFrameForSyncedTime(synced01);
    if (f == nullptr || !f->image.isValid())
        return;

    g.drawImageAt(f->image, 0, 0);
}

void GifSyncVSTAudioProcessorEditor::timerCallback()
{
    const auto t = nowSeconds();
    const auto dt = juce::jlimit(0.0, 0.1, t - lastTimerSec);
    lastTimerSec = t;

    if (!processor.getIsPlaying())
        freeRunPhaseSec += dt;

    repaint();
}

void GifSyncVSTAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
        showContextMenu();
}

void GifSyncVSTAudioProcessorEditor::showContextMenu()
{
    juce::PopupMenu m;

    m.addItem(1, "Load GIF...");
    {
        juce::File f(processor.getLastGifPath());
        if (f.existsAsFile())
            m.addItem(2, "Reload");
    }

    m.addSeparator();

    juce::PopupMenu syncMenu;

    auto addSyncItem = [&](int id, const juce::String& label, double barsValue)
    {
        const bool ticked = std::abs(syncBars - barsValue) < 0.000001;
        syncMenu.addItem(id, label, true, ticked);
    };

    addSyncItem(1001, "1/8 bar", 0.125);
    addSyncItem(1002, "1/4 bar", 0.25);
    addSyncItem(1003, "1/2 bar", 0.5);
    syncMenu.addSeparator();
    addSyncItem(1010, "1 bar", 1.0);
    addSyncItem(1011, "2 bars", 2.0);
    addSyncItem(1012, "3 bars", 3.0);
    addSyncItem(1013, "4 bars", 4.0);
    addSyncItem(1014, "5 bars", 5.0);
    addSyncItem(1015, "6 bars", 6.0);
    addSyncItem(1016, "7 bars", 7.0);
    addSyncItem(1017, "8 bars", 8.0);

    m.addSubMenu("Sync length", syncMenu);

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this](int r)
        {
            if (r == 1)
            {
                auto flags = juce::FileBrowserComponent::openMode
                           | juce::FileBrowserComponent::canSelectFiles;

                auto chooser = std::make_shared<juce::FileChooser>(
                    "Select a GIF",
                    [&]()
                    {
                        juce::File f(processor.getLastGifPath());
                        if (f.existsAsFile())
                            return f.getParentDirectory();
                        return juce::File::getSpecialLocation(juce::File::userHomeDirectory);
                    }(),
                    "*.gif");

                chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc)
                {
                    auto f = fc.getResult();
                    if (f.existsAsFile())
                        loadGifFromFile(f);
                });

                return;
            }

            if (r == 2)
            {
                juce::File f(processor.getLastGifPath());
                if (f.existsAsFile())
                    loadGifFromFile(f);
                return;
            }

            switch (r)
            {
                case 1001: syncBars = 0.125; break;
                case 1002: syncBars = 0.25;  break;
                case 1003: syncBars = 0.5;   break;
                case 1010: syncBars = 1.0;   break;
                case 1011: syncBars = 2.0;   break;
                case 1012: syncBars = 3.0;   break;
                case 1013: syncBars = 4.0;   break;
                case 1014: syncBars = 5.0;   break;
                case 1015: syncBars = 6.0;   break;
                case 1016: syncBars = 7.0;   break;
                case 1017: syncBars = 8.0;   break;
                default: return;
            }

            freeRunPhaseSec = 0.0;
            processor.setSyncBars(syncBars);
            repaint();
        });
}

void GifSyncVSTAudioProcessorEditor::loadGifFromFile(const juce::File& file)
{
    juce::FileInputStream in(file);
    if (!in.openedOk())
    {
        statusText = "Failed to open file";
        frames.clear();
        totalGifSec = 0.0;
        setSize(420, 280);
        return;
    }

    juce::MemoryBlock mb;
    in.readIntoMemoryBlock(mb);

    if (mb.getSize() == 0)
    {
        statusText = "Failed to read file";
        frames.clear();
        totalGifSec = 0.0;
        setSize(420, 280);
        return;
    }

    int w = 0, h = 0, framesCount = 0, comp = 0;
    int* delays = nullptr;

    unsigned char* pixels = stbi_load_gif_from_memory(
        (const unsigned char*) mb.getData(),
        (int) mb.getSize(),
        &delays,
        &w,
        &h,
        &framesCount,
        &comp,
        4
    );

    if (pixels == nullptr || w <= 0 || h <= 0 || framesCount <= 0)
    {
        if (pixels) stbi_image_free(pixels);
        if (delays) stbi_image_free(delays);

        statusText = "Failed to decode GIF";
        frames.clear();
        totalGifSec = 0.0;
        setSize(420, 280);
        return;
    }

    frames.clear();
    totalGifSec = 0.0;

    const int stride = w * h * 4;

    for (int fi = 0; fi < framesCount; ++fi)
    {
        const unsigned char* src = pixels + (size_t) fi * (size_t) stride;

        juce::Image img(juce::Image::ARGB, w, h, true);
        juce::Image::BitmapData bd(img, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < h; ++y)
        {
            auto* dst = bd.getLinePointer(y);
            const unsigned char* s = src + (size_t) y * (size_t) w * 4;

            for (int x = 0; x < w; ++x)
            {
                const unsigned char r = s[x * 4 + 0];
                const unsigned char g = s[x * 4 + 1];
                const unsigned char b = s[x * 4 + 2];
                const unsigned char a = s[x * 4 + 3];

                dst[x * 4 + 0] = b;
                dst[x * 4 + 1] = g;
                dst[x * 4 + 2] = r;
                dst[x * 4 + 3] = a;
            }
        }

        double durSec = 1.0 / 24.0;
        if (delays != nullptr)
        {
            const int ms = delays[fi];
            if (ms > 0) durSec = juce::jlimit(0.001, 10.0, (double) ms / 1000.0);
        }

        totalGifSec += durSec;

        GifFrame gf;
        gf.image = img;
        gf.endTimeSec = totalGifSec;
        frames.add(gf);
    }

    stbi_image_free(pixels);
    if (delays) stbi_image_free(delays);

    processor.setLastGifPath(file.getFullPathName());
    statusText = file.getFileName();

    const int ww = juce::jlimit(64, 4096, w);
    const int hh = juce::jlimit(64, 4096, h);

    freeRunPhaseSec = 0.0;
    setSize(ww, hh);
}
