#pragma once
// =============================================================================
//  KTDB-69 / Collateral Effects — grafica: colori, font incorporati, logo, testo giustificato
//  Font: TeX Gyre Adventor (GUST Font License, vedi resources/fonts/GUST-FONT-LICENSE.txt)
// =============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace ui
{
struct Theme
{
    juce::Colour text     { 0xffece6d2 };   // crema
    juce::Colour logo     { 0xfff6f4ee };   // bianco
    juce::Colour panelTop { 0xff42503a };   // verde oliva
    juce::Colour panelBot { 0xff2b3526 };
    juce::Colour bar      { 0xff1a1b19 };
    juce::Colour accent   { 0xffe39a3b };
    juce::Typeface::Ptr regular, bold;
    float tracking = 0.06f;

    Theme()
    {
        regular = juce::Typeface::createSystemTypefaceFor (BinaryData::texgyreadventorregular_otf,
                                                           BinaryData::texgyreadventorregular_otfSize);
        bold    = juce::Typeface::createSystemTypefaceFor (BinaryData::texgyreadventorbold_otf,
                                                           BinaryData::texgyreadventorbold_otfSize);
    }
};

inline const Theme& theme() { static const Theme t; return t; }

inline juce::Font font (float size, bool bold = true)
{
    const auto& t = theme();
    auto tf = bold ? t.bold : t.regular;
    if (tf == nullptr) return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    return juce::Font (juce::FontOptions (tf).withHeight (size * 0.98f).withKerningFactor (t.tracking));
}

//==============================================================================
// Bordi a sinistra e a destra dell'inchiostro di una scritta (non della sua larghezza tipografica)
inline juce::Range<float> inkExtent (const juce::String& text, const juce::Font& f)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, text, 0.0f, 0.0f);
    float l = 1.0e9f, r = -1.0e9f;
    for (int i = 0; i < ga.getNumGlyphs(); ++i)
    {
        const auto& gl = ga.getGlyph (i);
        if (gl.isWhitespace()) continue;
        juce::Path p; gl.createPath (p);
        if (p.isEmpty()) continue;
        // Path::getBounds() include i punti di controllo delle curve (una "C" risulterebbe più larga):
        // si misura il contorno vero, appiattito in segmenti
        for (juce::PathFlatteningIterator it (p, {}, 0.01f); it.next();)
        {
            l = juce::jmin (l, it.x1, it.x2);
            r = juce::jmax (r, it.x1, it.x2);
        }
    }
    return { l, r };
}

// Disegna "text" sulla linea di base y in modo che l'inchiostro vada esattamente da x0 a x1:
// lo spazio in più (o in meno) viene distribuito in parti uguali tra tutti i caratteri.
inline void drawJustified (juce::Graphics& g, const juce::String& text, const juce::Font& f,
                           float x0, float x1, float baselineY)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, text, 0.0f, baselineY);
    const int n = ga.getNumGlyphs();
    if (n == 0) return;
    const auto ink = inkExtent (text, f);
    const float extra = n > 1 ? ((x1 - x0) - ink.getLength()) / (float) (n - 1) : 0.0f;
    for (int i = 0; i < n; ++i)
        ga.moveRangeOfGlyphs (i, 1, x0 - ink.getStart() + extra * (float) i, 0.0f);
    g.setFont (f);
    ga.draw (g);
}

// Disegna una scritta a partire da x (bordo sinistro dell'inchiostro) e restituisce dove finisce
inline juce::Range<float> drawInkLeft (juce::Graphics& g, const juce::String& text, const juce::Font& f,
                                       float x, float baselineY)
{
    const auto ink = inkExtent (text, f);
    juce::GlyphArrangement ga;                       // posizione in virgola mobile: nessun arrotondamento
    ga.addLineOfText (f, text, x - ink.getStart(), baselineY);
    ga.draw (g);
    return { x, x + ink.getLength() };
}

//==============================================================================
// Logo Collateral Effects: tre curve passa-alto (tre scatti della manopola grande) sulla stessa base
inline void drawLogo (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col, float stroke)
{
    g.setColour (col);
    const juce::PathStrokeType S (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    auto in = r.reduced (r.getWidth() * 0.06f);

    for (int k = 0; k < 3; ++k)
    {
        const float shift = 0.30f + 0.30f * (float) k;
        juce::Path p;
        bool started = false;
        for (int i = 0; i <= 120; ++i)
        {
            const float x = (float) i / 120.0f;
            // asse logaritmico come sui grafici di risposta: rampa passa-alto con ginocchio morbido
            const float q = std::pow (10.0f, 3.0f * (shift - x));
            const float db = -10.0f * std::log10 (1.0f + q * q * q);
            const float y = 0.10f + 0.90f * juce::jmin (1.0f, -db / 30.0f);
            if (y > 0.995f && ! started) continue;
            if (x > shift + 0.10f) break;
            const juce::Point<float> pt (in.getX() + x * in.getWidth(), in.getY() + y * in.getHeight());
            if (! started) { p.startNewSubPath (pt); started = true; } else p.lineTo (pt);
        }
        g.strokePath (p, S);
    }
    g.drawLine (in.getX(), in.getBottom(), in.getRight(), in.getBottom(), stroke * 0.6f);
}

// Logo + "COLLATERAL" con sotto "EFFECTS" giustificato esattamente sulla larghezza della riga sopra.
// Il blocco è allineato a destra su "right"; h = altezza del logo.
inline void drawBrand (juce::Graphics& g, float right, float top, float h, juce::Colour col)
{
    const auto big = font (h * 0.50f, true), small = font (h * 0.27f, false);
    const auto ink = inkExtent ("COLLATERAL", big);
    const float textX = right - ink.getLength();
    drawLogo (g, { textX - h * 1.22f, top, h, h }, col, juce::jmax (1.4f, h * 0.06f));
    g.setColour (col);
    const auto line = drawInkLeft (g, "COLLATERAL", big, textX, top + h * 0.52f);
    drawJustified (g, "EFFECTS", small, line.getStart(), line.getEnd(), top + h * 0.92f);
}

// Icona dell'app Standalone: logo bianco su quadrato arrotondato verde
inline juce::Image makeIcon (int size)
{
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Graphics g (img);
    const auto& t = theme();
    auto r = juce::Rectangle<float> ((float) size, (float) size).reduced ((float) size * 0.1f);
    juce::ColourGradient bg (t.panelTop.brighter (0.15f), r.getX(), r.getY(), t.panelBot, r.getRight(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, (float) size * 0.18f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r.reduced (1.0f), (float) size * 0.18f, (float) size * 0.006f);
    drawLogo (g, r.reduced (r.getWidth() * 0.2f), t.logo, (float) size * 0.032f);
    return img;
}
} // namespace ui
