#include "PluginEditor.h"

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    using ui::font;

    juce::Path flutedCircle (juce::Point<float> c, float r, int flutes, float depth)
    {
        juce::Path p;
        const int n = flutes * 12;
        for (int i = 0; i <= n; ++i)
        {
            const float t = twoPi * (float) i / (float) n;
            const float rr = r - depth * (0.5f + 0.5f * std::cos ((float) flutes * t));
            const juce::Point<float> pt (c.x + rr * std::sin (t), c.y - rr * std::cos (t));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    void softShadow (juce::Graphics& g, juce::Point<float> c, float r)
    {
        for (int k = 8; k >= 0; --k)
        {
            const float rr = r + (float) k * r * 0.025f;
            g.setColour (juce::Colours::black.withAlpha (0.07f));
            g.fillEllipse (c.x - rr + r * 0.03f, c.y - rr + r * 0.10f, rr * 2, rr * 2);
        }
    }

    // alluminio tornito: riflessi conici fissi (la luce non gira con la manopola)
    void spunAluminium (juce::Graphics& g, juce::Point<float> c, float r)
    {
        const int n = 144;
        const juce::Rectangle<float> rc (c.x - r, c.y - r, 2 * r, 2 * r);
        for (int i = 0; i < n; ++i)
        {
            const float a0 = twoPi * (float) i / (float) n, a1 = twoPi * (float) (i + 1) / (float) n;
            const float t = 0.5f * (a0 + a1);
            float b = 0.62f + 0.24f * std::cos (2.0f * (t + 0.45f)) + 0.05f * std::cos (6.0f * t + 1.3f)
                    + 0.015f * std::sin (37.0f * t);
            b = juce::jlimit (0.2f, 0.97f, b);
            g.setColour (juce::Colour::fromFloatRGBA (b * 0.985f, b * 0.99f, b, 1.0f));
            juce::Path seg; seg.addPieSegment (rc, a0, a1 + 0.004f, 0.0f);
            g.fillPath (seg);
        }
        juce::ColourGradient vign (juce::Colours::transparentBlack, c.x, c.y,
                                   juce::Colours::black.withAlpha (0.28f), c.x + r, c.y, true);
        g.setGradientFill (vign); g.fillEllipse (rc);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawEllipse (rc, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawEllipse (rc.reduced (1.2f), 0.7f);
    }

    juce::Point<float> polarPt (juce::Point<float> c, float r, float deg)
    {
        const float a = juce::degreesToRadians (deg);
        return { c.x + r * std::sin (a), c.y - r * std::cos (a) };
    }

    void drawKnob (juce::Graphics& g, juce::Point<float> c, float r, float angleDeg, bool big)
    {
        softShadow (g, c, r);

        // gonna nera zigrinata
        auto skirt = flutedCircle (c, r, big ? 36 : 18, r * (big ? 0.03f : 0.06f));
        juce::ColourGradient sg (juce::Colour (0xff3a3a3a), c.x - r * 0.4f, c.y - r * 0.6f,
                                 juce::Colour (0xff070707), c.x + r * 0.5f, c.y + r * 0.8f, false);
        g.setGradientFill (sg); g.fillPath (skirt);
        g.setColour (juce::Colours::white.withAlpha (0.10f)); g.strokePath (skirt, juce::PathStrokeType (0.8f));

        // spalla
        const float rs = r * (big ? 0.80f : 0.74f);
        juce::ColourGradient shg (juce::Colour (0xff050505), c.x, c.y - rs, juce::Colour (0xff2b2b2b), c.x, c.y + rs, false);
        g.setGradientFill (shg); g.fillEllipse (c.x - rs, c.y - rs, rs * 2, rs * 2);

        // indice sulla gonna
        const auto p0 = polarPt (c, rs * 1.0f, angleDeg), p1 = polarPt (c, r * 0.97f, angleDeg);
        g.setColour (ui::theme().text);
        g.drawLine ({ p0, p1 }, r * (big ? 0.045f : 0.09f));

        // cappello in alluminio
        const float rc = r * (big ? 0.70f : 0.60f);
        spunAluminium (g, c, rc);
        if (! big)
        {
            g.setColour (juce::Colour (0xff111111));
            g.drawLine ({ polarPt (c, rc * 0.10f, angleDeg), polarPt (c, rc * 0.98f, angleDeg) }, r * 0.085f);
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawLine ({ polarPt (c, rc * 0.80f, angleDeg), polarPt (c, rc * 0.98f, angleDeg) }, r * 0.03f);
        }
    }
}

//==============================================================================
PanelLookAndFeel::PanelLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff222420));
    setColour (juce::PopupMenu::textColourId, ui::theme().text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::theme().panelTop);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, ui::theme().text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff222420));
    setColour (juce::TooltipWindow::textColourId, ui::theme().text);
}

void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const float r = (float) juce::jmin (w, h) * 0.5f - 4.0f;
    const juce::Point<float> c ((float) x + (float) w * 0.5f, (float) y + (float) h * 0.5f);
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r + 2.5f, r + 2.5f, 0, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.strokePath (arc, juce::PathStrokeType (2.0f));
    const float mid = startAngle + (endAngle - startAngle) * 0.5f, cur = startAngle + pos * (endAngle - startAngle);
    juce::Path val;
    val.addCentredArc (c.x, c.y, r + 2.5f, r + 2.5f, 0, juce::jmin (mid, cur), juce::jmax (mid, cur), true);
    g.setColour (ui::theme().accent);
    g.strokePath (val, juce::PathStrokeType (2.0f));
    drawKnob (g, c, r * 0.92f, juce::radiansToDegrees (cur), false);
}

//==============================================================================
SteppedKnob::SteppedKnob (juce::RangedAudioParameter& p, Style s, std::vector<float> anglesDeg,
                          std::vector<juce::String> lbls, juce::Point<float> centre, float r)
    : param (p),
      attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); }, nullptr),
      style (s), angles (std::move (anglesDeg)), labels (std::move (lbls)), c (centre), radius (r)
{
    attachment.sendInitialUpdate();
    setRepaintsOnMouseActivity (false);
}

juce::Point<float> SteppedKnob::polar (float r, float deg) const { return polarPt (c, r, deg); }

bool SteppedKnob::hitTest (int x, int y)
{
    const float lim = style == Style::Big ? radius + 50.0f : radius + 40.0f;
    return c.getDistanceFrom ({ (float) x, (float) y }) < lim;
}

void SteppedKnob::paint (juce::Graphics& g)
{
    const bool big = style == Style::Big;

    // scala
    g.setColour (ui::theme().text);
    for (size_t i = 0; i < angles.size(); ++i)
    {
        const float a = angles[i];
        if (big)
        {
            g.drawLine ({ polar (radius + 7.0f, a), polar (radius + 15.0f, a) }, 1.6f);
            const auto lp = polar (radius + 31.0f, a);
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (a * 0.55f), lp.x, lp.y));
            g.setFont (font (15.0f));
            g.drawText (labels[i], juce::Rectangle<float> (lp.x - 34, lp.y - 9, 68, 18), juce::Justification::centred);
        }
        else
        {
            g.drawLine ({ polar (radius + 5.0f, a), polar (radius + 11.0f, a) }, 1.4f);
            if (labels[i].isNotEmpty())
            {
                const auto lp = polar (radius + 24.0f, a);
                g.setFont (font (12.0f));
                const float w = 50.0f;
                auto just = juce::Justification::centred;
                juce::Rectangle<float> rc (lp.x - w * 0.5f, lp.y - 8, w, 16);
                if (a < -70.0f) { rc = { lp.x - w + 8, lp.y - 8, w, 16 }; just = juce::Justification::centredRight; }
                if (a > 70.0f)  { rc = { lp.x - 8, lp.y - 8, w, 16 };     just = juce::Justification::centredLeft; }
                g.drawText (labels[i], rc, just);
            }
        }
    }

    if (style == Style::Mode)
    {
        // arco "IN" sopra le posizioni attive (BELL, 600, NOTCH)
        const float ra = radius + 38.0f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, ra, ra, 0, juce::degreesToRadians (angles[1] - 4.0f),
                           juce::degreesToRadians (angles[3] + 6.0f), true);
        g.strokePath (arc, juce::PathStrokeType (1.4f));
        const auto lp = polar (ra, angles[1] - 14.0f);
        g.setFont (font (12.0f));
        g.drawText ("IN", juce::Rectangle<float> (lp.x - 22, lp.y - 8, 22, 16), juce::Justification::centredRight);
    }

    if (learning)
    {
        const float t = (float) (juce::Time::getMillisecondCounter() % 1000) / 1000.0f;
        g.setColour (ui::theme().accent.withAlpha (0.45f + 0.45f * std::sin (t * twoPi)));
        g.drawEllipse (c.x - radius - 5, c.y - radius - 5, (radius + 5) * 2, (radius + 5) * 2, 2.5f);
    }

    drawKnob (g, c, radius, angles[(size_t) juce::jlimit (0, (int) angles.size() - 1, index)], big);
}

int SteppedKnob::labelAt (juce::Point<float> p) const
{
    const float rl = style == Style::Big ? radius + 31.0f : radius + 24.0f;
    int best = -1; float bestD = style == Style::Big ? 22.0f : 18.0f;
    for (size_t i = 0; i < angles.size(); ++i)
    {
        const float d = polar (rl, angles[i]).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = (int) i; }
    }
    return best;
}

void SteppedKnob::setIndex (int i, bool notify)
{
    i = juce::jlimit (0, (int) angles.size() - 1, i);
    if (i == index) return;
    index = i;
    if (notify) attachment.setValueAsPartOfGesture ((float) i);
    repaint();
}

void SteppedKnob::mouseDown (const juce::MouseEvent& e)
{
    popupClick = e.mods.isPopupMenu();
    if (popupClick) { if (onLearnMenu) onLearnMenu(); return; }

    attachment.beginGesture();
    dragging = true;
    dragStartIndex = index;
    dragAccum = 0.0f;
    if (const int l = labelAt (e.position); l >= 0) setIndex (l, true);
}

void SteppedKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (popupClick || ! dragging) return;
    const float pxPerStep = style == Style::Big ? 14.0f : 22.0f;
    dragAccum = (float) (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY());
    if (std::abs (dragAccum) > 3.0f)
        setIndex (dragStartIndex + (int) std::trunc (dragAccum / pxPerStep + (dragAccum > 0 ? 0.5f : -0.5f)), true);
}

void SteppedKnob::mouseUp (const juce::MouseEvent&)
{
    if (popupClick) { popupClick = false; return; }
    if (dragging) { attachment.endGesture(); dragging = false; }
}

void SteppedKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    wheelAccum += (std::abs (w.deltaY) > std::abs (w.deltaX) ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    const int steps = (int) (wheelAccum / 0.12f);
    if (steps != 0)
    {
        wheelAccum -= (float) steps * 0.12f;
        attachment.setValueAsCompleteGesture ((float) juce::jlimit (0, (int) angles.size() - 1, index + steps));
    }
}

//==============================================================================
void LearnableSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onLearnMenu) onLearnMenu(); return; }
    Slider::mouseDown (e);
}

void ClickToggle::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onLearnMenu) onLearnMenu(); return; }
    ToggleButton::mouseDown (e);
}

void ClickToggle::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff2a2b28).brighter (highlighted ? 0.15f : 0.0f));
    g.fillRoundedRectangle (b, 5.0f);
    auto led = b.removeFromLeft (b.getHeight()).reduced (b.getHeight() * 0.32f);
    g.setColour (getToggleState() ? ui::theme().accent : juce::Colour (0xff4a4a44));
    g.fillEllipse (led);
    if (getToggleState()) { g.setColour (ui::theme().accent.withAlpha (0.3f)); g.fillEllipse (led.expanded (3.0f)); }
    g.setColour (ui::theme().text);
    g.setFont (font (12.0f));
    g.drawText (getToggleState() ? "CLICK SELETTORE" : "SENZA CLICK", b, juce::Justification::centredLeft);
}

//==============================================================================
PanelContent::PanelContent (DB69Processor& p) : proc (p)
{
    auto& ap = proc.apvts;
    const float py = (float) barH;

    // manopola grande: OFF … 7.5KC, 500 in alto
    {
        std::vector<float> ang; std::vector<juce::String> lab;
        for (int i = 0; i < db69::numSteps; ++i) { ang.push_back (-125.0f + 25.0f * (float) i); lab.push_back (db69::stepLabels[(size_t) i]); }
        const float R = 92.0f, half = R + 52.0f;
        bigKnob = std::make_unique<SteppedKnob> (*ap.getParameter (ParamIDs::freq), SteppedKnob::Style::Big,
                                                 ang, lab, juce::Point<float> (half, half), R);
        bigKnob->setBounds (juce::Rectangle<float> (2 * half, 2 * half).withCentre ({ 390.0f, py + 236.0f }).toNearestInt());
    }
    {
        const float R = 36.0f, half = R + 62.0f;
        modeKnob = std::make_unique<SteppedKnob> (*ap.getParameter (ParamIDs::mode), SteppedKnob::Style::Mode,
            std::vector<float> { -100.0f, -58.0f, -18.0f, 24.0f },
            std::vector<juce::String> { "OUT", "BELL", "600", "NOTCH" }, juce::Point<float> (half, half), R);
        modeKnob->setBounds (juce::Rectangle<float> (2 * half, 2 * half).withCentre ({ 150.0f, py + 306.0f }).toNearestInt());

        bellKnob = std::make_unique<SteppedKnob> (*ap.getParameter (ParamIDs::bell), SteppedKnob::Style::Bell,
            std::vector<float> { -60.0f, -30.0f, 0.0f, 30.0f, 60.0f, 90.0f },
            std::vector<juce::String> { "MIN", "", "", "", "", "MAX" }, juce::Point<float> (half, half), R);
        bellKnob->setBounds (juce::Rectangle<float> (2 * half, 2 * half).withCentre ({ 626.0f, py + 306.0f }).toNearestInt());
    }

    bigKnob->onLearnMenu  = [this] { showLearnMenu (DB69Processor::lFreq, bigKnob.get()); };
    modeKnob->onLearnMenu = [this] { showLearnMenu (DB69Processor::lMode, modeKnob.get()); };
    bellKnob->onLearnMenu = [this] { showLearnMenu (DB69Processor::lBell, bellKnob.get()); };
    for (auto* k : { bigKnob.get(), modeKnob.get(), bellKnob.get() })
        addAndMakeVisible (k);

    auto setupTrim = [this, &ap] (LearnableSlider& s, const char* id, int learnIdx, int x)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 18);
        s.setDoubleClickReturnValue (true, 0.0);
        s.setTextValueSuffix (" dB");
        s.onLearnMenu = [this, learnIdx, &s] { showLearnMenu (learnIdx, &s); };
        s.setBounds (x, 4, 106, 36);
        addAndMakeVisible (s);
        (learnIdx == DB69Processor::lInput ? inAtt : outAtt) =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (ap, id, s);
    };
    setupTrim (inSlider,  ParamIDs::input,  DB69Processor::lInput,  60);
    setupTrim (outSlider, ParamIDs::output, DB69Processor::lOutput, 236);

    clickBtn.setBounds (360, 9, 142, 26);
    clickBtn.onLearnMenu = [this] { showLearnMenu (DB69Processor::lClicks, &clickBtn); };
    clickBtn.setTooltip ("Selettore reale: interruzione del contatto e componenti che entrano scarichi (click), "
                         "oppure dissolvenza di 25 ms. Clic destro: MIDI Learn");
    addAndMakeVisible (clickBtn);
    clickAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (ap, ParamIDs::clicks, clickBtn);

    setSize (baseW, baseH);
}

PanelContent::~PanelContent() { proc.cancelLearn(); }

void PanelContent::refresh()
{
    const int learn = proc.getLearningParam();
    bigKnob->learning  = learn == DB69Processor::lFreq;
    modeKnob->learning = learn == DB69Processor::lMode || learn == DB69Processor::lInOut;
    bellKnob->learning = learn == DB69Processor::lBell;
    repaint (0, 0, baseW, barH);
    for (auto* k : { bigKnob.get(), modeKnob.get(), bellKnob.get() })
        if (k->learning || k->learning != k->wasLearning) { k->wasLearning = k->learning; k->repaint(); }
    if (learn >= 0 || learn != lastLearnShown) { lastLearnShown = learn; repaint(); }
}

juce::String PanelContent::ccText (int idx) const
{
    const int cc = proc.getCCForParam (idx);
    return cc >= 0 ? "CC" + juce::String (cc) : juce::String ("--");
}

void PanelContent::drawPanel (juce::Graphics& g)
{
    const auto area = juce::Rectangle<float> (0, (float) barH, (float) baseW, (float) (baseH - barH));
    g.setColour (juce::Colour (0xff121311)); g.fillRect (area);

    auto plate = area.reduced (10.0f);
    // ombra del pannello
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (plate.translated (0, 3), 12.0f);

    juce::ColourGradient pg (ui::theme().panelTop.brighter (0.10f), plate.getX(), plate.getY(),
                             ui::theme().panelBot, plate.getRight(), plate.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (plate, 12.0f);

    // vernice a buccia d'arancia
    juce::Random rnd (9069);
    for (int i = 0; i < 2600; ++i)
    {
        const float x = plate.getX() + rnd.nextFloat() * plate.getWidth();
        const float y = plate.getY() + rnd.nextFloat() * plate.getHeight();
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.035f + 0.03f * rnd.nextFloat()));
        g.fillRect (x, y, 1.3f, 1.3f);
    }
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.07f), plate.getX(), plate.getY(),
                                juce::Colours::transparentWhite, plate.getX() + plate.getWidth() * 0.5f, plate.getBottom(), false);
    g.setGradientFill (sheen); g.fillRoundedRectangle (plate, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (plate.reduced (0.5f), 12.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawRoundedRectangle (plate.expanded (0.5f), 12.5f, 1.0f);

    // viti
    for (auto pt : { plate.getTopLeft() + juce::Point<float> (18, 18), plate.getTopRight() + juce::Point<float> (-18, 18),
                     plate.getBottomLeft() + juce::Point<float> (18, -18), plate.getBottomRight() + juce::Point<float> (-18, -18) })
    {
        juce::ColourGradient sgr (juce::Colour (0xffd9d9d4), pt.x - 3, pt.y - 4, juce::Colour (0xff5d5d58), pt.x + 4, pt.y + 5, false);
        g.setGradientFill (sgr); g.fillEllipse (pt.x - 6, pt.y - 6, 12, 12);
        g.setColour (juce::Colour (0xff2b2b28));
        g.drawLine (pt.x - 4, pt.y + 2, pt.x + 4, pt.y - 2, 1.6f);
    }

    // scritte: "TYPE 9069 DUB FILTER" giustificato sulla stessa larghezza di "KTDB-69"
    g.setColour (ui::theme().text);
    g.setFont (font (12.0f, false));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("PASSIVE HPF  \xc2\xb7  600 OHM  \xc2\xb7  18 dB/OCT")), juce::Rectangle<float> (44, plate.getY() + 20, 320, 16), juce::Justification::left);
    const auto title = ui::drawInkLeft (g, "KTDB-69", font (36.0f), 44.0f, plate.getY() + 70.0f);
    ui::drawJustified (g, "TYPE 9069 DUB FILTER", font (11.0f, false), title.getStart(), title.getEnd(), plate.getY() + 88.0f);

    // marchio LULL Audio
    ui::drawBrand (g, juce::Rectangle<float> (plate.getRight() - 196, plate.getY() + 24, 170, 40), ui::theme().logo);

    g.setFont (font (14.0f));
    g.drawText ("MODE", juce::Rectangle<float> (100, (float) barH + 384, 100, 18), juce::Justification::centred);
    g.drawText ("BELL", juce::Rectangle<float> (576, (float) barH + 384, 100, 18), juce::Justification::centred);
    g.setFont (font (12.0f));
    g.drawText ("LOW CUT FREQUENCY", juce::Rectangle<float> (290, (float) barH + 384, 200, 16), juce::Justification::centred);
}

void PanelContent::paint (juce::Graphics& g)
{
    // barra di servizio
    g.setColour (ui::theme().bar); g.fillRect (0, 0, baseW, barH);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine (barH - 1, 0, (float) baseW);
    g.setColour (ui::theme().text.withAlpha (0.8f));
    g.setFont (font (11.0f));
    g.drawText ("INPUT",  juce::Rectangle<float> (12, 0, 46, (float) barH), juce::Justification::centredRight);
    g.drawText ("OUTPUT", juce::Rectangle<float> (176, 0, 58, (float) barH), juce::Justification::centredRight);

    // stato MIDI
    juce::String line1, line2;
    const int learn = proc.getLearningParam();
    if (learn >= 0)
    {
        line1 = "MIDI LEARN: " + juce::String (DB69Processor::learnableNames[(size_t) learn]);
        line2 = "muovi un controllo della tastiera...";
    }
    else
    {
        line1 = "FREQ " + ccText (DB69Processor::lFreq) + "  BELL " + ccText (DB69Processor::lBell)
              + "  MODE " + ccText (DB69Processor::lMode) + "  IN/OUT " + ccText (DB69Processor::lInOut);
        const int cc = proc.lastCC.load();
        line2 = cc >= 0 ? "ultimo: CC" + juce::String (cc) + " = " + juce::String (proc.lastCCValue.load())
                        : juce::String ("clic destro su un comando: MIDI Learn");
    }
    g.setColour (learn >= 0 ? ui::theme().accent : ui::theme().text.withAlpha (0.75f));
    g.setFont (font (11.0f));
    g.drawFittedText (line1, juce::Rectangle<int> (514, 5, 256, 16), juce::Justification::centredRight, 1, 0.7f);
    g.setFont (font (11.0f, false));
    g.drawFittedText (line2, juce::Rectangle<int> (514, 22, 256, 16), juce::Justification::centredRight, 1, 0.7f);

    drawPanel (g);
}

void PanelContent::paintOverChildren (juce::Graphics& g)
{
    const int learn = proc.getLearningParam();
    juce::Component* t = learn == DB69Processor::lInput ? (juce::Component*) &inSlider
                       : learn == DB69Processor::lOutput ? (juce::Component*) &outSlider
                       : learn == DB69Processor::lClicks ? (juce::Component*) &clickBtn : nullptr;
    if (t != nullptr)
    {
        g.setColour (ui::theme().accent);
        g.drawRoundedRectangle (t->getBounds().toFloat().expanded (2.0f), 5.0f, 2.0f);
    }
}

void PanelContent::showLearnMenu (int idx, juce::Component* target)
{
    juce::PopupMenu m;
    const int learning = proc.getLearningParam();
    const int cc = proc.getCCForParam (idx);
    const juce::String name (DB69Processor::learnableNames[(size_t) idx]);

    m.addSectionHeader (name + (cc >= 0 ? "  (CC " + juce::String (cc) + ")" : juce::String()));
    m.addItem (1, learning == idx ? "Annulla MIDI Learn" : "MIDI Learn (poi muovi un controllo)");
    m.addItem (2, "Rimuovi assegnazione", cc >= 0);
    if (idx == DB69Processor::lMode)
    {
        const int cio = proc.getCCForParam (DB69Processor::lInOut);
        m.addSeparator();
        m.addSectionHeader ("Pulsante IN/OUT" + (cio >= 0 ? "  (CC " + juce::String (cio) + ")" : juce::String()));
        m.addItem (3, learning == DB69Processor::lInOut ? "Annulla MIDI Learn" : "MIDI Learn pulsante IN/OUT");
        m.addItem (4, "Rimuovi assegnazione IN/OUT", cio >= 0);
    }
    m.addSeparator();
    m.addItem (9, "Ripristina assegnazioni predefinite");

    juce::Component::SafePointer<PanelContent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [safe, idx, learning] (int r)
                     {
                         if (safe == nullptr) return;
                         auto& p = safe->proc;
                         if (r == 1) { if (learning == idx) p.cancelLearn(); else p.startLearn (idx); }
                         if (r == 2) p.clearMappingForParam (idx);
                         if (r == 3) { if (learning == DB69Processor::lInOut) p.cancelLearn(); else p.startLearn (DB69Processor::lInOut); }
                         if (r == 4) p.clearMappingForParam (DB69Processor::lInOut);
                         if (r == 9) p.resetDefaultMappings();
                         safe->refresh();
                     });
}

//==============================================================================
DB69Editor::DB69Editor (DB69Processor& p) : AudioProcessorEditor (p), proc (p), content (p)
{
    setLookAndFeel (&lnf);
    content.setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    setResizable (true, true);
    setResizeLimits (PanelContent::baseW / 2, PanelContent::baseH / 2, PanelContent::baseW * 2, PanelContent::baseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) PanelContent::baseW / (double) PanelContent::baseH);
    setSize (PanelContent::baseW, PanelContent::baseH);
    startTimerHz (30);
}

DB69Editor::~DB69Editor()
{
    content.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DB69Editor::resized()
{
    const float s = (float) getWidth() / (float) PanelContent::baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, PanelContent::baseW, PanelContent::baseH);
}

void DB69Editor::timerCallback()
{
    const int learn = proc.getLearningParam();
    const int mv = proc.getMapVersion();
    const int cnt = proc.ccCounter.load();
    if (learn >= 0 || learn != lastLearn || mv != lastMapVersion || cnt != lastCCCount)
    {
        lastLearn = learn; lastMapVersion = mv; lastCCCount = cnt;
        content.refresh();
    }
}
