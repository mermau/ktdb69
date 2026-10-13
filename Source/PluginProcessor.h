#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <array>
#include <atomic>
#include "AltecNetwork.h"

namespace ParamIDs
{
    inline constexpr const char* freq   = "freq";
    inline constexpr const char* mode   = "mode";
    inline constexpr const char* bell   = "bell";
    inline constexpr const char* input  = "input";
    inline constexpr const char* output = "output";
    inline constexpr const char* clicks = "clicks";
    inline constexpr const char* bypass = "bypass";
}

//==============================================================================
class DB69Processor : public juce::AudioProcessor,
                      private juce::MidiInputCallback,
                      private juce::AsyncUpdater
{
public:
    DB69Processor();
    ~DB69Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    //==========================================================================
    // MIDI CC: mappa e MIDI Learn (thread-safe, senza lock)
    enum Learnable { lFreq = 0, lMode, lBell, lInOut, lInput, lOutput, lClicks, lBypass, numLearnable };
    static const std::array<const char*, numLearnable> learnableNames;

    void startLearn (int idx)        { learningParam.store (idx); }
    void cancelLearn()               { learningParam.store (-1); }
    int  getLearningParam() const    { return learningParam.load(); }
    int  getCCForParam (int idx) const;
    void clearMappingForParam (int idx);
    void resetDefaultMappings();
    int  getMapVersion() const       { return mapVersion.load(); }

    // IN/OUT: commuta tra OUT e l'ultimo modo attivo (usato dal pulsante MIDI)
    void toggleInOut();

    std::atomic<int> lastCC { -1 }, lastCCValue { 0 }, ccCounter { 0 };

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    //==========================================================================
    // Ingresso MIDI diretto: il plugin apre da solo le porte della tastiera, così i CC
    // arrivano anche su una traccia audio, senza instradamenti nella DAW (e anche in Logic).
    // "*" = tutti gli ingressi, "" = solo MIDI dalla DAW, altrimenti l'identificatore della porta.
    static inline const juce::String allInputs { "*" };
    void setMidiSource (const juce::String& source);
    juce::String getMidiSource() const;
    bool directMidiAvailable() const { return wrapperType != wrapperType_Standalone; }
    juce::StringArray getOpenMidiInputNames() const;

    // per i test: simula un messaggio arrivato dalla porta MIDI diretta e lo elabora subito
    void injectDirectMidiForTest (const juce::MidiMessage& m)
    {
        directActive.store (true);
        handleIncomingMidiMessage (nullptr, m);
        handleUpdateNowIfNeeded();
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;
    void handleAsyncUpdate() override;
    void reopenMidiInputs();
    void handleCC (int cc, int value);
    void setParamNorm (juce::RangedAudioParameter* p, float v);

    std::array<db69::FilterChannel, 2> channels;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::DryWetMixer<float> mixer { 512 };
    juce::SmoothedValue<float> inGainSm, outGainSm;

    std::atomic<float>* freqRaw = nullptr;
    std::atomic<float>* modeRaw = nullptr;
    std::atomic<float>* bellRaw = nullptr;
    std::atomic<float>* inRaw   = nullptr;
    std::atomic<float>* outRaw  = nullptr;
    std::atomic<float>* clickRaw = nullptr;
    std::atomic<float>* bypassRaw = nullptr;
    juce::AudioParameterBool* bypassParam = nullptr;

    // MIDI diretto: coda senza lock dal thread MIDI al thread dei messaggi
    juce::String midiSource { "*" };
    mutable juce::CriticalSection midiLock;
    std::vector<std::unique_ptr<juce::MidiInput>> midiInputs;
    juce::MidiDeviceListConnection deviceListConnection;
    juce::AbstractFifo ccFifo { 512 };
    std::array<std::pair<int, int>, 512> ccQueue {};
    juce::SpinLock ccHandlerLock;
    std::atomic<bool> directActive { false };

    std::array<juce::RangedAudioParameter*, numLearnable> learnParams {};
    std::array<std::atomic<int>, 128> ccToParam;
    std::array<bool, 128> ccHigh {};
    std::atomic<int> learningParam { -1 }, mapVersion { 0 };
    std::atomic<int> lastActiveMode { (int) db69::Mode::Bell };

    int maxBlock = 512, numChannels = 2;
    static constexpr float voltsPerFS = 10.954f;  // 0 dBFS = +18 dBu (EBU R68)

    JUCE_DECLARE_WEAK_REFERENCEABLE (DB69Processor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DB69Processor)
};
