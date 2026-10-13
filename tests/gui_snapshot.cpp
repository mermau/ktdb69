// Salva l'interfaccia del plugin in PNG (uso: gui_snapshot out.png [scala] [freq mode bell])
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    DB69Processor proc;
    if (argc >= 6)
    {
        auto set = [&] (const char* id, int v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 ((float) v)); };
        set (ParamIDs::freq, std::atoi (argv[3])); set (ParamIDs::mode, std::atoi (argv[4])); set (ParamIDs::bell, std::atoi (argv[5]));
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;
    ed->setSize (juce::roundToInt (PanelContent::baseW * scale), juce::roundToInt (PanelContent::baseH * scale));
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
