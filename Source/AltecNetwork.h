#pragma once
// =============================================================================
//  DB-69 — modello circuitale del filtro passa-alto passivo Altec 9069B
//  (C++17, nessuna dipendenza: usato sia dal plugin sia dai test)
//
//  Rete a T "costante-K" da 600 ohm (come le unità Langevin/Altec dell'epoca):
//
//      Vin ─ Rs ─ A ──||── B ──||── C ─┬─ uscita
//                     C1    │    C2    RL
//                           L (+ r)     │
//                           │          ⏚
//                           ⏚
//
//  C1 = C2 = kC / (2π·fc·R0),  L = kL · R0 / (4π·fc),  R0 = 600 Ω
//  kL e kC sono le tolleranze dei componenti di ogni scatto, ricavate dalle curve
//  misurate (taglio in 600, picco BELL e minimo NOTCH di ogni scatto: tre misure,
//  due incognite, scarto ≤ 0.5 % tra 70 Hz e 3 kHz).
//  Con Rs = RL = 600 Ω la risposta è una Butterworth del 3° ordine (18 dB/ott)
//  con 6 dB di perdita di inserzione: la risposta originale del 9069B.
//
//  r = resistenza in continua dell'avvolgimento (toroide con prese:
//  r ∝ N ∝ √L), è ciò che limita il Q in BELL e la profondità in NOTCH.
//  Il nucleo satura dolcemente: i = φ/L · (1 + (φ/φs)²), con φs ∝ N.
//
//  Modi (come il KTBK-1B, che fornisce i valori di riferimento):
//   600   : sorgente e carico 600 Ω, risposta originale (−6 dB)
//   BELL  : sorgente a bassa impedenza (uscita del banco) e carico "a ponte":
//           la rete non è terminata e risuona (il suono di King Tubby).
//           Il livello BELL sceglie il carico (DF0..DF5).
//   NOTCH : gli stessi componenti ricollegati in un ramo serie LC verso massa
//           (C1‖C2 in serie a L): reiezione centrata sul valore della manopola,
//           profondità limitata da r. Il livello BELL aggiunge smorzamento.
//
//  Integrazione: analisi nodale a 3 nodi, condensatori e induttore con la regola
//  dei trapezi "pre-deformata" (prewarping) sulla risonanza dello scatto scelto:
//  quella frequenza cade esattamente dove deve anche a 44.1 kHz, senza oversampling
//  (quindi senza latenza e senza filtri anti-aliasing nel percorso del segnale).
//  La non linearità dell'induttore è risolta con Newton su una sola variabile
//  (equivalente di Thevenin visto dall'induttore).
// =============================================================================

#include <array>
#include <cmath>
#include <complex>

namespace db69
{

enum class Mode : int { Out = 0, Bell = 1, Line600 = 2, Notch = 3 };

constexpr int numSteps   = 11;   // 0 = OFF, poi 70 Hz … 7.5 kHz
constexpr int numDamping = 6;    // DF0 … DF5 (manopola BELL, MIN → MAX)
constexpr int numModes   = 4;

inline constexpr std::array<double, numSteps> stepFreqs {
    0.0, 70.0, 100.0, 150.0, 250.0, 500.0, 1000.0, 2000.0, 3000.0, 5000.0, 7500.0 };

inline constexpr std::array<const char*, numSteps> stepLabels {
    "OFF", "70", "100", "150", "250", "500", "1KC", "2KC", "3KC", "5KC", "7.5KC" };

inline constexpr std::array<const char*, numModes> modeLabels { "OUT", "BELL", "600", "NOTCH" };

//==============================================================================
// Valori dei componenti. Tutta la taratura è qui.
struct Design
{
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double R0 = 600.0;     // impedenza caratteristica
    static constexpr double r70 = 51.0;     // resistenza dell'avvolgimento alla presa dei 70 Hz [Ω]
    static constexpr double phiSat70 = 0.09;// flusso di saturazione alla presa dei 70 Hz [V·s]

    // Tolleranze di induttore e condensatori per ogni scatto (indice 1..10), tarate sulle curve
    // misurate dell'unità di riferimento: agli scatti bassi i componenti reali sono più grandi
    // del nominale (+14…20 % sul prodotto L·C), a quelli alti il condensatore domina.
    static constexpr std::array<double, numSteps> kL { 1.0, 1.142, 1.074, 1.104, 1.054, 1.034, 1.037, 1.052, 0.967, 0.987, 0.966 };
    static constexpr std::array<double, numSteps> kC { 1.0, 1.045, 1.126, 1.033, 1.057, 1.084, 1.038, 0.998, 1.138, 1.137, 1.177 };

    // 600: terminazione originale
    static constexpr double lineRs = 600.0, lineRL = 600.0;
    // BELL: uscita del banco a bassa impedenza, carico scelto dal livello BELL (tarato sui 6 livelli misurati)
    static constexpr double bellRs = 40.0;
    static constexpr std::array<double, numDamping> bellRL { 512.0, 845.0, 1311.0, 1754.0, 2493.0, 3744.0 };
    // NOTCH: build-out 600 Ω, carico a ponte, smorzamento in serie al ramo LC
    static constexpr double notchRs = 600.0, notchRL = 20000.0;
    static constexpr std::array<double, numDamping> notchRd { 150.0, 100.0, 47.0, 22.0, 10.0, 0.0 };

    // valori per lo scatto "step" (1..10); r e saturazione dipendono dalle spire della presa (nominali)
    static double C (int step)      { return kC[(size_t) step] / (2.0 * pi * stepFreqs[(size_t) step] * R0); }
    static double L (int step)      { return kL[(size_t) step] * R0 / (4.0 * pi * stepFreqs[(size_t) step]); }
    static double r (int step)      { return r70 * std::sqrt (70.0 / stepFreqs[(size_t) step]); }
    static double phiSat (int step) { return phiSat70 * std::sqrt (70.0 / stepFreqs[(size_t) step]); }
};

struct Config
{
    Mode mode = Mode::Bell;
    int step = 0;      // 0..10
    int damping = 5;   // 0..5
    bool operator== (const Config& o) const { return mode == o.mode && step == o.step && damping == o.damping; }
    bool operator!= (const Config& o) const { return ! (*this == o); }
};

//==============================================================================
// Risposta in frequenza teorica (piccolo segnale) — usata dai test e dal display
inline std::complex<double> analyticResponse (const Config& c, double f)
{
    using cd = std::complex<double>;
    const cd s (0.0, 2.0 * Design::pi * f);
    if (c.mode == Mode::Out) return 1.0;

    double Rs = Design::lineRs, RL = Design::lineRL;
    if (c.mode == Mode::Bell)  { Rs = Design::bellRs;  RL = Design::bellRL[(size_t) c.damping]; }
    if (c.mode == Mode::Notch) { Rs = Design::notchRs; RL = Design::notchRL; }
    if (c.step == 0) return RL / (Rs + RL);

    const double C = Design::C (c.step), L = Design::L (c.step), r = Design::r (c.step);
    const cd zl = s * L + r;

    if (c.mode == Mode::Notch)
    {
        const cd zsh = 1.0 / (s * 2.0 * C) + zl + Design::notchRd[(size_t) c.damping];
        const cd zp = 1.0 / (1.0 / zsh + 1.0 / RL);
        return zp / (zp + Rs);
    }
    const cd zc = 1.0 / (s * C);
    const cd zb = 1.0 / (1.0 / zl + 1.0 / (zc + RL));
    const cd vb = zb / (zc + zb + Rs);
    return vb * RL / (zc + RL);
}

//==============================================================================
class Network
{
public:
    void prepare (double sampleRate) { T = 1.0 / sampleRate; rebuild(); reset(); }

    // Cambia i componenti. Se keepState = false i nuovi componenti partono scarichi
    // (come quando il selettore collega altri condensatori/prese).
    void configure (const Config& c, bool sourceOpen = false)
    {
        cfg = c; open = sourceOpen;
        rebuild();
    }
    void setSourceOpen (bool o) { if (o != open) { open = o; rebuild(); } }
    const Config& config() const { return cfg; }

    void reset()
    {
        for (auto& cp : caps) { cp.v = 0.0; cp.i = 0.0; }
        phi = 0.0; vLp = 0.0;
    }

    // copia lo stato fisico (tensioni dei condensatori, flusso) da un'altra rete
    void copyStateFrom (const Network& o)
    {
        for (size_t k = 0; k < caps.size(); ++k) { caps[k].v = o.caps[k].v; caps[k].i = o.caps[k].i; }
        phi = o.phi; vLp = o.vLp;
    }

    // ingresso e uscita in volt
    double process (double vin)
    {
        if (cfg.mode == Mode::Out) return vin;

        // iniezioni di corrente (equivalenti di Norton)
        double b[3] = { open ? 0.0 : Gs * vin, 0.0, 0.0 };
        for (int k = 0; k < numCaps; ++k)
        {
            auto& cp = caps[(size_t) k];
            cp.Ieq = cp.G * cp.v + cp.i;
            b[cp.p] += cp.Ieq;
            if (cp.q >= 0) b[cp.q] -= cp.Ieq;
        }

        double v[3];
        for (int n = 0; n < 3; ++n)
            v[n] = Z[n][0] * b[0] + Z[n][1] * b[1] + Z[n][2] * b[2];

        if (hasL)
        {
            // Thevenin visto dall'induttore (nodo B → massa), r in serie: Voc - Rt·i
            const double Voc  = v[1];
            const double hist = phi + halfT * vLp;

            // soluzione lineare; Newton solo se il nucleo si avvicina alla saturazione
            double ph = (hist + halfT * Voc) * linDen;
            double x2 = ph * invPhiS; x2 *= x2;
            if (x2 > 1.0e-6)   // sotto: la correzione è < 1e-6 relativo (−120 dB), inutile calcolarla
            {
                for (int it = 0; it < 8; ++it)
                {
                    const double g  = ph * invL * (1.0 + x2);
                    const double dg = invL * (1.0 + 3.0 * x2);
                    const double F  = ph - hist - halfT * (Voc - Rt * g);
                    const double step = F / (1.0 + halfT * Rt * dg);
                    ph -= step;
                    x2 = ph * invPhiS; x2 *= x2;
                    if (std::abs (step) <= 1.0e-12 + 1.0e-9 * std::abs (ph)) break;
                }
            }
            const double iL = ph * invL * (1.0 + x2);
            vLp = Voc - Rt * iL;
            phi = ph;
            for (int n = 0; n < 3; ++n) v[n] -= Z[n][1] * iL;
        }

        for (int k = 0; k < numCaps; ++k)
        {
            auto& cp = caps[(size_t) k];
            const double vc = v[cp.p] - (cp.q >= 0 ? v[cp.q] : 0.0);
            cp.i = cp.G * vc - cp.Ieq;
            cp.v = vc;
        }
        return v[outNode];
    }

private:
    struct Cap { int p = 0, q = -1; double C = 0, G = 0, v = 0, i = 0, Ieq = 0; };

    void rebuild()
    {
        double M[3][3] = {};
        auto addG = [&M] (int p, int q, double g)
        {
            M[p][p] += g;
            if (q >= 0) { M[q][q] += g; M[p][q] -= g; M[q][p] -= g; }
        };

        const double leak = 1.0e-9;
        for (int n = 0; n < 3; ++n) M[n][n] += leak;

        numCaps = 0; hasL = false; outNode = 0; halfT = 0.5 * T;
        double Rs = Design::lineRs, RL = Design::lineRL;
        if (cfg.mode == Mode::Bell)  { Rs = Design::bellRs;  RL = Design::bellRL[(size_t) cfg.damping]; }
        if (cfg.mode == Mode::Notch) { Rs = Design::notchRs; RL = Design::notchRL; }
        Gs = 1.0 / Rs;
        if (! open) addG (0, -1, Gs);

        if (cfg.mode != Mode::Out && cfg.step > 0)
        {
            Lval = Design::L (cfg.step);
            rL = Design::r (cfg.step);
            invPhiS = 1.0 / Design::phiSat (cfg.step);
            hasL = true;
            const double C = Design::C (cfg.step);
            const bool notch = cfg.mode == Mode::Notch;

            // passo di integrazione pre-deformato sulla risonanza L–C serie della rete
            const double w0 = 1.0 / std::sqrt (Lval * (notch ? 2.0 * C : C));
            halfT = std::tan (w0 * 0.5 * T) / w0;

            if (notch)
            {
                setCap (0, 0, 1, 2.0 * C);           // C1‖C2 in serie al ramo L
                rL += Design::notchRd[(size_t) cfg.damping];
                addG (0, -1, 1.0 / RL);
                outNode = 0;
            }
            else
            {
                setCap (0, 0, 1, C);                  // C1: A–B
                setCap (1, 1, 2, C);                  // C2: B–C
                addG (2, -1, 1.0 / RL);
                outNode = 2;
            }
        for (int k = 0; k < numCaps; ++k)
                addG (caps[(size_t) k].p, caps[(size_t) k].q, caps[(size_t) k].G);
        }
        else
        {
            addG (0, -1, 1.0 / RL);                   // OFF: solo partitore Rs/RL
            outNode = 0;
        }
        invert3 (M, Z);
        // costanti per l'induttore, calcolate una volta per configurazione
        invL   = 1.0 / Lval;
        Rt     = Z[1][1] + rL;
        linDen = 1.0 / (1.0 + halfT * Rt * invL);
    }

    void setCap (int idx, int p, int q, double C)
    {
        auto& cp = caps[(size_t) idx];
        cp.p = p; cp.q = q; cp.C = C; cp.G = C / halfT;
        numCaps = idx + 1;
    }

    static void invert3 (const double m[3][3], double out[3][3])
    {
        const double a = m[0][0], b = m[0][1], c = m[0][2],
                     d = m[1][0], e = m[1][1], f = m[1][2],
                     g = m[2][0], h = m[2][1], i = m[2][2];
        const double A =  (e * i - f * h), B = -(d * i - f * g), C =  (d * h - e * g);
        const double det = a * A + b * B + c * C;
        const double id = 1.0 / det;
        out[0][0] = A * id;  out[0][1] = -(b * i - c * h) * id; out[0][2] =  (b * f - c * e) * id;
        out[1][0] = B * id;  out[1][1] =  (a * i - c * g) * id; out[1][2] = -(a * f - c * d) * id;
        out[2][0] = C * id;  out[2][1] = -(a * h - b * g) * id; out[2][2] =  (a * e - b * d) * id;
    }

    Config cfg;
    bool open = false;
    double T = 1.0 / 192000.0;
    double Z[3][3] {};
    double Gs = 1.0 / 600.0;
    std::array<Cap, 2> caps {};
    int numCaps = 0, outNode = 0;
    bool hasL = false;
    double Lval = 1.0, rL = 0.0, invPhiS = 1.0, phi = 0.0, vLp = 0.0;
    double halfT = 0.5 / 192000.0, invL = 1.0, Rt = 0.0, linDen = 1.0;
};

//==============================================================================
// Un canale con il comportamento del selettore rotativo:
//  - clicks = true : il contatto si apre (≈0.6–1.6 ms), poi rimbalza 1–3 volte prima di
//    chiudersi ("clicks and crunches" del banco di Tubby); i nuovi componenti entrano
//    scarichi → click e transitori reali. La sequenza dei rimbalzi cambia a ogni scatto
//    ma è la stessa per i due canali (è un solo selettore).
//  - clicks = false: le due reti girano in parallelo e si incrociano in 25 ms
class FilterChannel
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (auto& n : net) { n.prepare (sampleRate); n.configure (current); n.reset(); }
        fadeLen = (int) std::lround (0.025 * fs);
        fadePos = -1; segLeft = 0; numSegs = 0; target = current;
    }

    void reset()
    {
        for (auto& n : net) { n.reset(); n.setSourceOpen (false); }
        fadePos = -1; segLeft = 0; numSegs = 0;
        if (target != current) apply();
    }

    void setConfig (const Config& c, bool clicks)
    {
        target = c; targetClicks = clicks;
        if (fadePos < 0) apply();   // altrimenti si applica alla fine della dissolvenza
    }

    double process (double vin)
    {
        if (segLeft > 0 && --segLeft == 0) nextSegment();

        double y = net[(size_t) a].process (vin);
        if (fadePos >= 0)
        {
            const double t = (double) fadePos / (double) fadeLenCur;
            const double yo = net[(size_t) fadeFrom].process (vin);
            y = yo + (y - yo) * t;
            if (++fadePos >= fadeLenCur) { fadePos = -1; apply(); }
        }
        return y;
    }

    const Config& config() const { return current; }

private:
    void apply()
    {
        if (target == current) return;
        const Config c = target;
        const bool bothActive = current.mode != Mode::Out && c.mode != Mode::Out;
        current = c;

        if (targetClicks && bothActive)
        {
            // contatto aperto, poi i nuovi componenti (scarichi) entrano nel circuito
            net[(size_t) a].configure (c, true);
            net[(size_t) a].reset();
            makeBounce();
            return;
        }
        // dissolvenza tra la rete attuale e quella nuova
        const int nb = 1 - a;
        net[(size_t) nb].configure (c);
        if (net[(size_t) a].config().mode == c.mode && c.mode != Mode::Out)
            net[(size_t) nb].copyStateFrom (net[(size_t) a]);
        else
            net[(size_t) nb].reset();
        fadeFrom = a; a = nb; fadePos = 0; segLeft = 0; numSegs = 0;
        // con i click attivi l'ingresso/uscita (relè true bypass) è quasi istantaneo
        fadeLenCur = targetClicks ? atLeastOne ((int) std::lround (0.002 * fs)) : fadeLen;
    }

    // durate alternate aperto/chiuso/aperto/... (in campioni); l'ultima fase è sempre chiusa
    void makeBounce()
    {
        unsigned st = 0x9069u + 2654435761u * ++events;          // stesso seme per i due canali
        auto rnd = [&st] (double lo, double hi)
        {
            st = st * 1664525u + 1013904223u;
            return lo + (hi - lo) * (double) (st >> 8) / 16777216.0;
        };
        auto ms = [this] (double t) { return atLeastOne ((int) std::lround (t * 0.001 * fs)); };
        numSegs = 0;
        seg[(size_t) numSegs++] = ms (rnd (0.6, 1.6));            // apertura principale
        const int bounces = 1 + (int) rnd (0.0, 3.0);
        for (int b = 0; b < bounces && numSegs + 2 <= (int) seg.size(); ++b)
        {
            seg[(size_t) numSegs++] = ms (rnd (0.08, 0.6));       // chiuso
            seg[(size_t) numSegs++] = ms (rnd (0.02, 0.2));       // rimbalzo (aperto)
        }
        segIdx = 0; segLeft = seg[0];
    }

    void nextSegment()
    {
        if (++segIdx < numSegs)
        {
            net[(size_t) a].setSourceOpen (segIdx % 2 == 0);
            segLeft = seg[(size_t) segIdx];
        }
        else
        {
            net[(size_t) a].setSourceOpen (false);
            numSegs = 0;
        }
    }

    static int atLeastOne (int v) { return v < 1 ? 1 : v; }

    std::array<Network, 2> net;
    Config current, target;
    bool targetClicks = true;
    double fs = 48000.0;
    int a = 0, fadeFrom = 1, fadePos = -1, fadeLen = 1200, fadeLenCur = 1200;
    std::array<int, 8> seg {};
    int numSegs = 0, segIdx = 0, segLeft = 0;
    unsigned events = 0;
};

} // namespace db69
