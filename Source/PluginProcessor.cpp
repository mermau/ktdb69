#include "PluginProcessor.h"
#include "PluginEditor.h"

const std::array<const char*, DB69Processor::numLearnable> DB69Processor::learnableNames {
    "FREQUENZA", "MODE", "BELL", "IN/OUT", "INPUT", "OUTPUT", "CLICK"
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DB69Processor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray freqs, modes;
    for (auto* s : db69::stepLabels) freqs.add (s);
    for (auto* s : db69::modeLabels) modes.add (s);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::freq, 1 }, "Frequency", freqs, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode", modes, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::bell, 1 }, "Bell",
                                                         StringArray { "MIN", "2", "3", "4", "5", "MAX" }, 3));
    auto trim = [] (const char* id, const char* name)
    {
        return std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
            AudioParameterFloatAttributes().withLabel ("dB"));
    };
    layout.add (trim (ParamIDs::input, "Input"));
    layout.add (trim (ParamIDs::output, "Output"));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::clicks, 1 }, "Switch Clicks", true));
    return layout;
}

DB69Processor::DB69Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    freqRaw  = apvts.getRawParameterValue (ParamIDs::freq);
    modeRaw  = apvts.getRawParameterValue (ParamIDs::mode);
    bellRaw  = apvts.getRawParameterValue (ParamIDs::bell);
    inRaw    = apvts.getRawParameterValue (ParamIDs::input);
    outRaw   = apvts.getRawParameterValue (ParamIDs::output);
    clickRaw = apvts.getRawParameterValue (ParamIDs::clicks);

    learnParams[lFreq]   = apvts.getParameter (ParamIDs::freq);
    learnParams[lMode]   = apvts.getParameter (ParamIDs::mode);
    learnParams[lBell]   = apvts.getParameter (ParamIDs::bell);
    learnParams[lInOut]  = nullptr;   // azione, non parametro
    learnParams[lInput]  = apvts.getParameter (ParamIDs::input);
    learnParams[lOutput] = apvts.getParameter (ParamIDs::output);
    learnParams[lClicks] = apvts.getParameter (ParamIDs::clicks);

    resetDefaultMappings();
}

//==============================================================================
void DB69Processor::resetDefaultMappings()
{
    for (auto& m : ccToParam) m.store (-1);
    // CC "liberi" per le manopole, CC 64 (pedale sustain / pulsante) per IN/OUT
    ccToParam[20].store (lFreq);
    ccToParam[21].store (lBell);
    ccToParam[22].store (lMode);
    ccToParam[64].store (lInOut);
    ++mapVersion;
}

int DB69Processor::getCCForParam (int idx) const
{
    for (int cc = 0; cc < 128; ++cc)
        if (ccToParam[(size_t) cc].load() == idx) return cc;
    return -1;
}

void DB69Processor::clearMappingForParam (int idx)
{
    for (auto& m : ccToParam)
        if (m.load() == idx) m.store (-1);
    ++mapVersion;
}

void DB69Processor::setParamNorm (juce::RangedAudioParameter* p, float v)
{
    if (p == nullptr || std::abs (p->getValue() - v) < 1.0e-6f) return;
    p->beginChangeGesture();
    p->setValueNotifyingHost (v);
    p->endChangeGesture();
}

void DB69Processor::toggleInOut()
{
    auto* p = learnParams[lMode];
    const int cur = (int) modeRaw->load();
    const int next = cur == (int) db69::Mode::Out ? lastActiveMode.load() : (int) db69::Mode::Out;
    setParamNorm (p, p->convertTo0to1 ((float) next));
}

void DB69Processor::handleCC (int cc, int value)
{
    lastCC.store (cc); lastCCValue.store (value); ++ccCounter;

    int learn = learningParam.load();
    if (learn >= 0 && learningParam.compare_exchange_strong (learn, -1))
    {
        for (auto& m : ccToParam)
            if (m.load() == learn) m.store (-1);
        ccToParam[(size_t) cc].store (learn);
        ccHigh[(size_t) cc] = value >= 64;
        ++mapVersion;
        return; // il messaggio usato per l'apprendimento non modifica il parametro
    }

    const int idx = ccToParam[(size_t) cc].load();
    if (idx < 0) return;

    // pulsanti: commutano alla pressione (KeyLab: pulsante in modalità Gate/Momentary)
    if (idx == lInOut || idx == lClicks)
    {
        const bool high = value >= 64;
        if (high && ! ccHigh[(size_t) cc])
        {
            if (idx == lInOut) toggleInOut();
            else setParamNorm (learnParams[lClicks], learnParams[lClicks]->getValue() > 0.5f ? 0.0f : 1.0f);
        }
        ccHigh[(size_t) cc] = high;
        return;
    }

    auto* p = learnParams[(size_t) idx];
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p))
    {
        // selettore a scatti: isteresi per non far "tremolare" il contatto tra due posizioni
        const int n = choice->choices.size();
        const float pos = (float) value / 127.0f * (float) (n - 1);
        const int cur = choice->getIndex();
        if (std::abs (pos - (float) cur) > 0.65f || value == 0 || value == 127)
        {
            const int next = juce::jlimit (0, n - 1, juce::roundToInt (pos));
            setParamNorm (p, p->convertTo0to1 ((float) next));
        }
    }
    else
    {
        setParamNorm (p, (float) value / 127.0f);
    }
}

//==============================================================================
bool DB69Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == layouts.getMainInputChannelSet();
}

void DB69Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    numChannels = juce::jlimit (1, 2, getTotalNumOutputChannels());
    maxBlock = juce::jmax (1, samplesPerBlock);

    // la rete gira sempre a 176-192 kHz: niente "warping" della bilineare sui picchi alti
    const int order = sampleRate > 120000.0 ? 0 : (sampleRate > 60000.0 ? 1 : 2);
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, (size_t) order,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) maxBlock);

    const double osRate = sampleRate * (double) oversampling->getOversamplingFactor();
    const db69::Config cfg { (db69::Mode) (int) modeRaw->load(), (int) freqRaw->load(), (int) bellRaw->load() };
    for (auto& ch : channels)
    {
        ch.prepare (osRate);
        ch.setConfig (cfg, false);
        ch.reset();
    }

    const auto latency = oversampling->getLatencyInSamples();
    setLatencySamples (juce::roundToInt (latency));

    mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    mixer.prepare ({ sampleRate, (juce::uint32) maxBlock, (juce::uint32) numChannels });
    mixer.setWetLatency (latency);
    mixer.setWetMixProportion (cfg.mode == db69::Mode::Out ? 0.0f : 1.0f);
    mixer.reset();

    inGainSm.reset (sampleRate, 0.03);
    outGainSm.reset (sampleRate, 0.03);
    inGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inRaw->load()));
    outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outRaw->load()));
}

void DB69Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController()) handleCC (m.getControllerNumber(), m.getControllerValue());
    }
    midi.clear();

    const int numCh = juce::jmin (getTotalNumInputChannels(), numChannels);
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, buffer.getNumSamples());
    if (numCh < 1) return;

    const db69::Config cfg { (db69::Mode) (int) modeRaw->load(), (int) freqRaw->load(), (int) bellRaw->load() };
    if (cfg.mode != db69::Mode::Out) lastActiveMode.store ((int) cfg.mode);
    const bool clicks = clickRaw->load() > 0.5f;
    for (int ch = 0; ch < numCh; ++ch) channels[(size_t) ch].setConfig (cfg, clicks);

    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (inRaw->load()));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (outRaw->load()));
    mixer.setWetMixProportion (cfg.mode == db69::Mode::Out ? 0.0f : 1.0f);   // OUT = true bypass

    juce::dsp::AudioBlock<float> full (buffer);
    full = full.getSubsetChannelBlock (0, (size_t) numCh);

    // l'host può inviare blocchi più grandi di quanto dichiarato: si lavora a pezzi
    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxBlock)
    {
        const int n = juce::jmin (maxBlock, buffer.getNumSamples() - offset);
        auto block = full.getSubBlock ((size_t) offset, (size_t) n);

        mixer.pushDrySamples (block);

        for (int i = 0; i < n; ++i)
        {
            const float g = inGainSm.getNextValue() * voltsPerFS;
            for (int ch = 0; ch < numCh; ++ch) block.getChannelPointer ((size_t) ch)[i] *= g;
        }

        auto up = oversampling->processSamplesUp (block);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = up.getChannelPointer ((size_t) ch);
            auto& fc = channels[(size_t) ch];
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = (float) fc.process ((double) d[i]);
        }
        oversampling->processSamplesDown (block);

        for (int i = 0; i < n; ++i)
        {
            const float g = outGainSm.getNextValue() / voltsPerFS;
            for (int ch = 0; ch < numCh; ++ch) block.getChannelPointer ((size_t) ch)[i] *= g;
        }

        mixer.mixWetSamples (block);
    }
}

//==============================================================================
void DB69Processor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("DB69State");
    if (auto params = apvts.copyState().createXml()) root.addChildElement (params.release());

    auto* map = root.createNewChildElement ("MIDIMAP");
    for (int cc = 0; cc < 128; ++cc)
        if (const int idx = ccToParam[(size_t) cc].load(); idx >= 0)
        {
            auto* e = map->createNewChildElement ("CC");
            e->setAttribute ("num", cc);
            e->setAttribute ("target", idx);
        }
    copyXmlToBinary (root, destData);
}

void DB69Processor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("DB69State")) return;

    if (auto* params = xml->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));

    if (auto* map = xml->getChildByName ("MIDIMAP"))
    {
        for (auto& m : ccToParam) m.store (-1);
        for (auto* e : map->getChildWithTagNameIterator ("CC"))
        {
            const int cc = e->getIntAttribute ("num", -1);
            const int idx = e->getIntAttribute ("target", -1);
            if (juce::isPositiveAndBelow (cc, 128) && juce::isPositiveAndBelow (idx, (int) numLearnable))
                ccToParam[(size_t) cc].store (idx);
        }
        ++mapVersion;
    }
}

juce::AudioProcessorEditor* DB69Processor::createEditor() { return new DB69Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DB69Processor(); }
