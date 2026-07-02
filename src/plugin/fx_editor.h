/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "JuceHeader.h"

#include "border_bounds_constrainer.h"
#include "fx_plugin.h"
#include "full_interface.h"
#include "synth_gui_interface.h"

class FxEditor : public AudioProcessorEditor, public SynthGuiInterface {
  public:
    FxEditor(FxPlugin&);

    void paint(Graphics&) override { }
    void resized() override;
    void setScaleFactor(float newScale) override;

    void updateFullGui() override;

  private:
    FxPlugin& synth_;
    BorderBoundsConstrainer constrainer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEditor)
};
