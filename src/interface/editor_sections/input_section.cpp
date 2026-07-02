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

#include "input_section.h"

#include "skin.h"
#include "synth_slider.h"

InputSection::InputSection(String name) : SynthSection(std::move(name)) {
  source_ = std::make_unique<SynthSlider>("input_1_source");
  addSlider(source_.get());
  source_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  source_->setBipolar(true);

  pan_ = std::make_unique<SynthSlider>("input_1_pan");
  addSlider(pan_.get());
  pan_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  pan_->setBipolar(true);

  width_ = std::make_unique<SynthSlider>("input_1_width");
  addSlider(width_.get());
  width_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  width_->setBipolar(true);

  level_ = std::make_unique<SynthSlider>("input_1_level");
  addSlider(level_.get());
  level_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  setSkinOverride(Skin::kOscillator);
}

InputSection::~InputSection() = default;

void InputSection::paintBackground(Graphics& g) {
  if (getWidth() == 0)
    return;

  paintContainer(g);
  paintHeadingText(g);
  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
  paintBorder(g);

  setLabelFont(g);
  drawLabelForComponent(g, TRANS("SOURCE"), source_.get());
  drawLabelForComponent(g, TRANS("PAN"), pan_.get());
  drawLabelForComponent(g, TRANS("WIDTH"), width_.get());
  drawLabelForComponent(g, TRANS("LEVEL"), level_.get());
}

void InputSection::resized() {
  SynthSection::resized();

  int title_width = getTitleWidth();
  int widget_margin = getWidgetMargin();
  int available_width = std::max(0, getWidth() - title_width - 2 * widget_margin);
  int controls_width = std::min(available_width, static_cast<int>(std::round(getStandardKnobSize() * 4.5f)));
  Rectangle<int> controls_bounds(title_width + widget_margin, widget_margin,
                                 controls_width, getHeight() - 2 * widget_margin);
  Rectangle<int> top_row = controls_bounds.removeFromTop(controls_bounds.getHeight() / 2);

  placeKnobsInArea(top_row, { source_.get(), pan_.get() });
  placeKnobsInArea(controls_bounds, { width_.get(), level_.get() });
}
