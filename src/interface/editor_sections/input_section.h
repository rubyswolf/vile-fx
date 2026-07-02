#pragma once

#include "JuceHeader.h"
#include "synth_section.h"

class SynthSlider;

class InputSection : public SynthSection {
  public:
    InputSection(String name);
    ~InputSection() override;

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;

  private:
    std::unique_ptr<SynthSlider> source_;
    std::unique_ptr<SynthSlider> pan_;
    std::unique_ptr<SynthSlider> width_;
    std::unique_ptr<SynthSlider> level_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputSection)
};
