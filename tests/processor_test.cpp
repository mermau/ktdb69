// Test del plugin completo (processore JUCE): CC MIDI, livelli, bypass
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <cstdio>

static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok " : "ERR", what); if (! ok) ++fails; }

static double runSine (DB69Processor& p, double fs, double f, double ampDb, int blocks, juce::MidiBuffer* midi = nullptr)
{
    const int bs = 512; juce::AudioBuffer<float> buf (2, bs); double peak = 0; static long n = 0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < bs; ++i, ++n)
        {
            const float x = (float) (std::pow (10.0, ampDb / 20) * std::sin (2 * juce::MathConstants<double>::pi * f * (double) n / fs));
            buf.setSample (0, i, x); buf.setSample (1, i, x);
        }
        juce::MidiBuffer m; if (midi != nullptr && b == 0) m = *midi;
        p.processBlock (buf, m);
        if (b > blocks / 2) for (int i = 0; i < bs; ++i) peak = std::max (peak, (double) std::abs (buf.getSample (0, i)));
    }
    return 20 * std::log10 (peak + 1e-12);
}

static void cc (DB69Processor& p, int num, int val)
{
    juce::AudioBuffer<float> buf (2, 64); buf.clear(); juce::MidiBuffer m;
    m.addEvent (juce::MidiMessage::controllerEvent (1, num, val), 0);
    p.processBlock (buf, m);
}
static int idx (DB69Processor& p, const char* id) { return (int) p.apvts.getRawParameterValue (id)->load(); }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000;
    DB69Processor p;
    p.setPlayConfigDetails (2, 2, fs, 512);
    p.prepareToPlay (fs, 512);
    std::printf ("Latenza: %d campioni\n", p.getLatencySamples());

    // CC20 → frequenza, con isteresi
    cc (p, 20, 127); check (idx (p, ParamIDs::freq) == 10, "CC20=127 → 7.5KC");
    cc (p, 20, 0);   check (idx (p, ParamIDs::freq) == 0,  "CC20=0 → OFF");
    int last = 0; bool mono = true, all = true; std::vector<int> seen (11, 0);
    for (int v = 0; v <= 127; ++v) { cc (p, 20, v); int i = idx (p, ParamIDs::freq); if (i < last) mono = false; last = i; seen[(size_t) i] = 1; }
    for (int s : seen) all &= s == 1;
    check (mono && all, "CC20 0→127 attraversa tutte le 11 posizioni in ordine");
    cc (p, 20, 64); int a = idx (p, ParamIDs::freq); cc (p, 20, 63); cc (p, 20, 64); cc (p, 20, 63);
    check (idx (p, ParamIDs::freq) == a, "isteresi: il valore oscillante non fa scattare il selettore");

    cc (p, 22, 127); check (idx (p, ParamIDs::mode) == 3, "CC22=127 → NOTCH");
    cc (p, 22, 40);  check (idx (p, ParamIDs::mode) == 1, "CC22=40 → BELL");
    cc (p, 21, 0);   check (idx (p, ParamIDs::bell) == 0, "CC21=0 → BELL MIN");
    cc (p, 64, 127); cc (p, 64, 0);  check (idx (p, ParamIDs::bypass) == 1, "CC64 premuto → BYPASS acceso");
    cc (p, 64, 127); cc (p, 64, 0);  check (idx (p, ParamIDs::bypass) == 0, "CC64 di nuovo → BYPASS spento");

    auto set = [&] (const char* id, int v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 ((float) v)); };

    // livelli
    set (ParamIDs::mode, 2); set (ParamIDs::freq, 0);
    double g = runSine (p, fs, 1000, -12, 120) + 12;
    std::printf ("600 / OFF a 1 kHz: %+.2f dB\n", g); check (std::abs (g + 6.0) < 0.2, "600: perdita di inserzione 6 dB");
    set (ParamIDs::freq, 6);
    g = runSine (p, fs, 1000, -12, 120) + 12;
    std::printf ("600 / 1KC a 1 kHz: %+.2f dB\n", g); check (std::abs (g + 9.0) < 0.4, "600: -3 dB (rel.) alla frequenza di taglio");
    auto peakOf = [] (int step) { double bf = 0, bg = -1e9;
        for (double f = 500; f < 15000; f *= 1.001) { const double v = 20 * std::log10 (std::abs (db69::analyticResponse ({ db69::Mode::Bell, step, 5 }, f))); if (v > bg) { bg = v; bf = f; } }
        return std::make_pair (bf, bg); };
    set (ParamIDs::mode, 1); set (ParamIDs::bell, 5);
    auto pk = peakOf (6);
    g = runSine (p, fs, pk.first, -24, 120) + 24;
    std::printf ("BELL MAX / 1KC a %.0f Hz: %+.2f dB (atteso %+.2f)\n", pk.first, g, pk.second); check (std::abs (g - pk.second) < 0.3, "BELL: picco risonante come il modello");
    set (ParamIDs::freq, 10);
    pk = peakOf (10);
    g = runSine (p, fs, pk.first, -24, 120) + 24;
    std::printf ("BELL MAX / 7.5KC a %.0f Hz (48 kHz, oversampling 4x): %+.2f dB (atteso %+.2f)\n", pk.first, g, pk.second); check (std::abs (g - pk.second) < 0.6, "BELL alto: niente deformazione della bilineare");
    set (ParamIDs::mode, 3); set (ParamIDs::freq, 6);
    g = runSine (p, fs, 964, -12, 120) + 12;
    std::printf ("NOTCH MAX / 1KC a 964 Hz: %+.2f dB\n", g); check (g < -25, "NOTCH profondo");

    // bypass: OUT = segnale invariato
    set (ParamIDs::mode, 0);
    g = runSine (p, fs, 300, -6, 200) + 6;
    std::printf ("OUT a 300 Hz: %+.3f dB\n", g); check (std::abs (g) < 0.05, "OUT: true bypass a guadagno unitario");

    // bypass: segnale invariato anche con il filtro su 1KC
    set (ParamIDs::mode, 2); set (ParamIDs::freq, 6); set (ParamIDs::bypass, 1);
    g = runSine (p, fs, 300, -6, 200) + 6;
    std::printf ("BYPASS (600 / 1KC) a 300 Hz: %+.3f dB\n", g); check (std::abs (g) < 0.05, "BYPASS: true bypass");
    set (ParamIDs::bypass, 0);
    check (p.getBypassParameter() == p.apvts.getParameter (ParamIDs::bypass), "il bypass della DAW comanda lo switch BYPASS");

    // coerenza di fase del MIX: 600 / OFF (filtro = 0.5·x). Con MIX 50 % l'uscita deve essere
    // esattamente 1.5 volte quella con MIX 100 % a ogni frequenza: nessun pettine
    {
        DB69Processor q; q.setPlayConfigDetails (2, 2, fs, 512); q.prepareToPlay (fs, 512);
        auto setq = [&] (const char* id, float v) { auto* r = q.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        setq (ParamIDs::mode, 2); setq (ParamIDs::freq, 0);
        double worst = 0, droop = 0;
        for (double f : { 50.0, 200.0, 1000.0, 3000.0, 6000.0, 10000.0, 14000.0, 18000.0, 20000.0 })
        {
            setq (ParamIDs::mix, 100.0f); const double g100 = runSine (q, fs, f, -12, 60) + 12;
            setq (ParamIDs::mix, 50.0f);  const double g50  = runSine (q, fs, f, -12, 60) + 12;
            worst = std::max (worst, std::abs (g50 - g100 - 20 * std::log10 (1.5)));
            droop = std::max (droop, std::abs (g100 + 6.0206));
        }
        std::printf ("MIX 50%%: scarto massimo dalla somma in fase %.3f dB (20 Hz-20 kHz); oversampling: %.2f dB a 20 kHz al massimo\n", worst, droop);
        check (worst < 0.05, "MIX: filtrato e diretto restano in fase (niente pettine dall'oversampling)");
    }

    // MIDI diretto dalla tastiera (senza passare dalla DAW) + MIDI Learn
    {
        DB69Processor q;
        q.setPlayConfigDetails (2, 2, fs, 512); q.prepareToPlay (fs, 512);
        q.injectDirectMidiForTest (juce::MidiMessage::controllerEvent (1, 20, 127));
        check (idx (q, ParamIDs::freq) == 10, "MIDI diretto: CC20 → 7.5KC");
        q.startLearn (DB69Processor::lBell);
        q.injectDirectMidiForTest (juce::MidiMessage::controllerEvent (1, 74, 10));
        q.injectDirectMidiForTest (juce::MidiMessage::controllerEvent (1, 74, 127));
        check (q.getCCForParam (DB69Processor::lBell) == 74 && idx (q, ParamIDs::bell) == 5, "MIDI Learn da porta diretta: CC74 → BELL");
        cc (q, 74, 0);
        check (idx (q, ParamIDs::bell) == 5, "con il MIDI diretto attivo i CC della DAW sono ignorati (niente doppioni)");
    }

    // stato salvato e ricaricato (mappatura MIDI compresa)
    juce::MemoryBlock mb; p.getStateInformation (mb);
    DB69Processor p2; p2.setStateInformation (mb.getData(), (int) mb.getSize());
    check (p2.getCCForParam (DB69Processor::lFreq) == 20 && idx (p2, ParamIDs::freq) == 6, "stato e mappa MIDI ripristinati");

    // prestazioni: 10 s stereo a 48 kHz, blocchi da 512, BELL MAX su 1KC, rumore rosa-ish a -12 dBFS
    {
        DB69Processor b; b.setPlayConfigDetails (2, 2, fs, 512); b.prepareToPlay (fs, 512);
        auto setb = [&] (const char* id, int v) { auto* q = b.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 ((float) v)); };
        setb (ParamIDs::mode, 1); setb (ParamIDs::freq, 6); setb (ParamIDs::bell, 5);
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; juce::Random rnd (1);
        const int blocks = (int) (10 * fs / 512);
        double t = 0;
        for (int k = 0; k < blocks; ++k)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.25f * (rnd.nextFloat() * 2 - 1));
            const auto t0 = juce::Time::getHighResolutionTicks();
            b.processBlock (buf, m);
            t += juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
        }
        std::printf ("CPU: %.2f%% di un core (10 s stereo a 48 kHz in %.3f s)\n", 100.0 * t / 10.0, t);
        setb (ParamIDs::bypass, 1); t = 0;
        for (int k = 0; k < blocks; ++k)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.25f * (rnd.nextFloat() * 2 - 1));
            const auto t0 = juce::Time::getHighResolutionTicks();
            b.processBlock (buf, m);
            t += juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
        }
        std::printf ("CPU in BYPASS: %.2f%% di un core\n", 100.0 * t / 10.0);
    }

    std::printf (fails ? "FALLITO (%d)\n" : "TUTTO OK\n", fails);
    return fails;
}
