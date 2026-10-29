// Test del modello DB-69: risposta simulata vs teorica, distorsione, click, CPU
#include "AltecNetwork.h"
#include <chrono>
#include <cstdio>
#include <vector>
#include <cstring>
using namespace db69;

static double sineGainDb (Network& n, double fs, double f, double amp)
{
    n.reset();
    const int settle = (int) (fs * std::max (0.25, 40.0 / f)), meas = (int) (fs * std::max (0.1, 20.0 / f));
    double re = 0, im = 0;
    const double w = 2 * Design::pi * f / fs;
    for (int i = 0; i < settle + meas; ++i)
    {
        const double y = n.process (amp * std::sin (w * i));
        if (i >= settle) { re += y * std::cos (w * i); im += y * std::sin (w * i); }
    }
    return 20 * std::log10 (2 * std::sqrt (re * re + im * im) / meas / amp);
}

static double thd (Network& n, double fs, double f, double amp)
{
    n.reset();
    const int settle = (int) (fs * 0.5); const int per = (int) std::lround (fs / f);
    const int meas = per * (int) std::ceil (0.2 * f);
    std::vector<double> y (meas);
    const double w = 2 * Design::pi * f / fs;
    for (int i = 0; i < settle + meas; ++i) { double v = n.process (amp * std::sin (w * i)); if (i >= settle) y[i - settle] = v; }
    auto h = [&] (int k) { double re = 0, im = 0; for (int i = 0; i < meas; ++i) { re += y[i] * std::cos (k * w * (i + settle)); im += y[i] * std::sin (k * w * (i + settle)); } return std::sqrt (re * re + im * im); };
    double h1 = h (1), hs = 0; for (int k = 2; k <= 7; ++k) hs += h (k) * h (k);
    return 100 * std::sqrt (hs) / h1;
}

int main (int argc, char** argv)
{
    const double fs = 48000.0;
    const double vFS = 7.746 * std::sqrt (2.0);   // 0 dBFS = +18 dBu
    Network n; n.prepare (fs);
    int fails = 0;

    // 1) simulazione vs teoria (piccolo segnale) alle frequenze di campionamento reali, senza oversampling
    for (double rate : { 176400.0, 192000.0 })   // la rete gira sempre qui (oversampling 4x o 2x)
    {
        Network nr; nr.prepare (rate);
        double maxErr = 0, maxPk = 0;
        for (int mode = 1; mode <= 3; ++mode)
            for (int st = 0; st < numSteps; ++st)
                for (int d = 0; d < numDamping; d += (mode == 2 ? 6 : 1))
                {
                    Config c { (Mode) mode, st, d }; nr.configure (c);
                    for (double f : { 30.0, 60.0, 120.0, 300.0, 700.0, 1500.0, 3000.0, 6000.0, 10000.0, 14000.0 })
                    {
                        const double sim = sineGainDb (nr, rate, f, 0.01);
                        const double th = 20 * std::log10 (std::abs (analyticResponse (c, f)));
                        if (th > -40) maxErr = std::max (maxErr, std::abs (sim - th));
                    }
                    if (mode == 1 && st > 0)   // picco BELL: frequenza e altezza
                    {
                        double bf = 0, bg = -1e9;
                        for (double f = 50; f < 15000; f *= 1.002) { double v = 20 * std::log10 (std::abs (analyticResponse (c, f))); if (v > bg) { bg = v; bf = f; } }
                        maxPk = std::max (maxPk, std::abs (sineGainDb (nr, rate, bf, 0.01) - bg));
                    }
                }
        std::printf ("%.1f kHz: errore max simulazione vs teoria fino a 14 kHz %.3f dB, sui picchi BELL %.3f dB\n", rate / 1000, maxErr, maxPk);
        if (maxErr > 0.3 || maxPk > 0.1) ++fails;
    }

    // 2) punti di controllo rispetto alle curve pubblicate
    auto peakOf = [] (Config c, bool minimum) { double best = minimum ? 1e9 : -1e9, bf = 0;
        for (double f = 20; f < 20000; f *= 1.002) { double g = 20 * std::log10 (std::abs (analyticResponse (c, f)));
            if (minimum ? g < best : g > best) { best = g; bf = f; } } return std::make_pair (bf, best); };
    for (int st : { 1, 6, 10 })
    {
        auto b = peakOf ({ Mode::Bell, st, 5 }, false), nt = peakOf ({ Mode::Notch, st, 5 }, true);
        std::printf ("%-5s  BELL max: picco %+.1f dB a %.0f Hz (%.2f·fc)   NOTCH max: %.1f dB a %.0f Hz\n",
                     stepLabels[st], b.second, b.first, b.first / stepFreqs[st], nt.second, nt.first);
    }
    std::printf ("600 @ fc (1 kHz): %.1f dB, banda passante %.1f dB\n",
                 20 * std::log10 (std::abs (analyticResponse ({ Mode::Line600, 6, 0 }, 1000))),
                 20 * std::log10 (std::abs (analyticResponse ({ Mode::Line600, 6, 0 }, 15000))));

    // 3) distorsione del nucleo
    for (double dBu : { 4.0, 18.0, 24.0, 30.0 })
    {
        const double amp = 0.7746 * std::sqrt (2.0) * std::pow (10.0, dBu / 20.0);
        n.configure ({ Mode::Line600, 1, 0 });  double t600 = thd (n, fs, 100, amp);
        n.configure ({ Mode::Bell, 1, 5 });     double tb = thd (n, fs, 100, amp);
        n.configure ({ Mode::Bell, 6, 5 });     double tb1k = thd (n, fs, 1500, amp);
        std::printf ("THD %+5.0f dBu: 600/70Hz@100Hz %.3f%%  BELL/70Hz@100Hz %.3f%%  BELL/1KC@1.5kHz %.4f%%\n", dBu, t600, tb, tb1k);
    }

    // 3b) aliasing della saturazione senza oversampling: THD a 48 kHz contro 192 kHz
    {
        Network a48, a192; a48.prepare (48000.0); a192.prepare (192000.0);
        const double amp = 0.7746 * std::sqrt (2.0) * std::pow (10.0, 24.0 / 20.0);
        a48.configure ({ Mode::Bell, 1, 5 }); a192.configure ({ Mode::Bell, 1, 5 });
        const double t48 = thd (a48, 48000.0, 100, amp), t192 = thd (a192, 192000.0, 100, amp);
        std::printf ("THD +24 dBu BELL/70 Hz a 100 Hz: %.3f%% a 48 kHz, %.3f%% a 192 kHz\n", t48, t192);
        if (std::abs (t48 - t192) > 0.05 * t192) ++fails;
    }

    // 4) click del selettore: sinusoide 200 Hz a -12 dBFS, passaggio 150 → 250 in BELL max
    for (int clicks = 0; clicks < 2; ++clicks)
    {
        FilterChannel ch; ch.prepare (fs); ch.setConfig ({ Mode::Bell, 3, 5 }, clicks); ch.reset();
        for (int i = 0; i < 20000; ++i) ch.process (0);
        double pkBefore = 0, pkAfter = 0;
        const double amp = 0.25 * vFS, w = 2 * Design::pi * 200 / fs;
        for (int i = 0; i < (int) fs; ++i)
        {
            if (i == (int) (fs / 2)) ch.setConfig ({ Mode::Bell, 4, 5 }, clicks);
            double y = ch.process (amp * std::sin (w * i));
            if (i > fs / 4 && i < fs / 2) pkBefore = std::max (pkBefore, std::abs (y));
            if (i >= fs / 2 && i < fs / 2 + fs / 20) pkAfter = std::max (pkAfter, std::abs (y));
            if (std::isnan (y)) { std::printf ("NaN!\n"); ++fails; break; }
        }
        std::printf ("Cambio 150→250 (%s): picco prima %.2f V, picco nei 50 ms dopo %.2f V\n", clicks ? "click" : "dissolvenza", pkBefore, pkAfter);
    }

    // 5) stabilità a livelli estremi e CPU
    {
        FilterChannel ch; ch.prepare (fs);
        unsigned seed = 1; double acc = 0; const int N = (int) fs * 10;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i)
        {
            if (i % 9600 == 0) ch.setConfig ({ (Mode) (1 + (i / 9600) % 3), (i / 9600) % 11, (i / 19200) % 6 }, (i / 9600) % 2);
            seed = seed * 1664525u + 1013904223u;
            acc += ch.process (4.0 * vFS * ((seed >> 8) / 8388608.0 - 1.0));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        const double sec = std::chrono::duration<double> (t1 - t0).count();
        std::printf ("10 s a 48 kHz (rumore +12 dBFS, cambi continui): %.3f s CPU (%.1f%% di un core per canale) %s\n",
                     sec, 100 * sec / 10, std::isfinite (acc) ? "stabile" : "INSTABILE");
        if (! std::isfinite (acc)) ++fails;
    }

    // 6) CSV per i grafici
    if (argc > 1)
    {
        FILE* fp = std::fopen (argv[1], "w");
        std::fprintf (fp, "mode,step,damp,f,db\n");
        for (int mode = 1; mode <= 3; ++mode) for (int st = 1; st < numSteps; ++st) for (int d = 0; d < numDamping; ++d)
            for (double f = 20; f <= 20000; f *= 1.02)
                std::fprintf (fp, "%d,%d,%d,%.2f,%.3f\n", mode, st, d, f, 20 * std::log10 (std::abs (analyticResponse ({ (Mode) mode, st, d }, f))));
        std::fclose (fp);
    }
    std::printf (fails ? "FALLITO\n" : "OK\n");
    return fails;
}
