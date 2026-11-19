// Render the plugin editor to a PNG without opening a window (for GUI review).
// Usage: trichorus_snapshot <output.png> [scale]

#include "PluginEditor.h"
#include "PluginProcessor.h"

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File out (juce::File::getCurrentWorkingDirectory()
                              .getChildFile (argc > 1 ? argv[1] : "snapshot.png"));
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 2.0f;

    TriChorusProcessor processor;
    processor.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (FrontPanel::kWidth, FrontPanel::kHeight);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);

    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
        return 1;

    std::printf ("Wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    return 0;
}
