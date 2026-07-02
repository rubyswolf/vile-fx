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

#include "chorus_module.h"

#include "delay.h"
#include "memory.h"
#include "synth_constants.h"

namespace vital {

  // Constructor for the ChorusModule class
  ChorusModule::ChorusModule(const Output* beats_per_second) :
      SynthModule(0, 1), beats_per_second_(beats_per_second),
      frequency_(nullptr), delay_time_1_(nullptr), delay_time_2_(nullptr),
      mod_depth_(nullptr), phase_(0.0f) {
    wet_ = 0.0f;
    dry_ = 0.0f;
    last_num_voices_ = 0;
    int max_samples = kMaxChorusDelay * kMaxSampleRate + 1;
        
    // Initialize delay lines
    for (int i = 0; i < kMaxDelayPairs; ++i) {
      registerOutput(&delay_status_outputs_[i]);
      delays_[i] = new MultiDelay(max_samples);
      addIdleProcessor(delays_[i]);
    }
  }

  // Initialize the chorus module
  void ChorusModule::init() {
    static const cr::Value kDelayStyle(MultiDelay::kMono);

    voices_ = createBaseControl("chorus_voices");

    Output* free_frequency = createMonoModControl("chorus_frequency");
    frequency_ = createTempoSyncSwitch("chorus", free_frequency->owner, beats_per_second_, false);
    Output* feedback = createMonoModControl("chorus_feedback");
    wet_output_ = createMonoModControl("chorus_dry_wet");
    Output* cutoff = createMonoModControl("chorus_cutoff");
    Output* spread = createMonoModControl("chorus_spread");
    mod_depth_ = createMonoModControl("chorus_mod_depth");

    delay_time_1_ = createMonoModControl("chorus_delay_1");
    delay_time_2_ = createMonoModControl("chorus_delay_2");

    // Plug controls into delay lines
    for (int i = 0; i < kMaxDelayPairs; ++i) {
      delays_[i]->plug(&delay_frequencies_[i], MultiDelay::kFrequency);
      delays_[i]->plug(feedback, MultiDelay::kFeedback);
      delays_[i]->plug(&constants::kValueOne, MultiDelay::kWet);
      delays_[i]->plug(cutoff, MultiDelay::kFilterCutoff);
      delays_[i]->plug(spread, MultiDelay::kFilterSpread);
      delays_[i]->plug(&kDelayStyle, MultiDelay::kStyle);
    }

    SynthModule::init();
  }

  // Enable or disable the chorus module
  void ChorusModule::enable(bool enable) {
    SynthModule::enable(enable);
    process(1);
    if (enable) {
      wet_ = 0.0f;
      dry_ = 0.0f;

      // Reset delay lines
      for (int i = 0; i < kMaxDelayPairs; ++i)
        delays_[i]->hardReset();
    }
  }

  // Get the number of voice pairs for the chorus effect
  int ChorusModule::getNextNumVoicePairs() {
    int num_voice_pairs = voices_->value();

    // Reset delay lines for new voices
    for (int i = last_num_voices_; i < num_voice_pairs; ++i)
      delays_[i]->reset(constants::kFullMask);

    last_num_voices_ = num_voice_pairs;
    return num_voice_pairs;
  }

  // Process the input audio with the chorus effect
  void ChorusModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);

    //Sets the frequency variable to the first value in the output buffer
    poly_float frequency = frequency_->buffer[0];

    //Reciprocal of the sample rate is the sample period
    //Multiplying this by the number of samples in the block gives the block period
    //Then the frequency multiplier is added
    poly_float delta_phase = (frequency * num_samples) * (1.0f / getSampleRate());

    //Step the phase by the delta phase and wrap it around
    phase_ = utils::mod(phase_ + delta_phase);

    //Get a pointer to the output buffer
    poly_float* audio_out = output()->buffer;

    for (int s = 0; s < num_samples; ++s) {
      //Conditionally sets the sample to one from the input buffer
      poly_float sample = audio_in[s] & constants::kFirstMask;

      //Adds the sample to the output buffer
      //Swap voices appears to be some sort of bit manipulation to prepare for SIMD operations
      audio_out[s] = sample + utils::swapVoices(sample);
    }

    int num_voices = getNextNumVoicePairs();

    //Get parameters from the modulation buffers?
    poly_float delay1 = delay_time_1_->buffer[0];

    poly_float delay2 = delay_time_2_->buffer[0];

    //Selects parts of delay 1 or 2 based on a bit mask
    poly_float delay_time = utils::maskLoad(delay2, delay1, constants::kFirstMask);
    poly_float average_delay = (delay_time + utils::swapVoices(delay_time)) * 0.5f;

    for (int i = 0; i < num_voices; ++i) { //For each voice

      //Calculate phase
      float pair_offset = i * 0.25f / num_voices;
      poly_float right_offset = (poly_float(0.25f) & constants::kRightMask);
      poly_float phase = phase_ + right_offset + (poly_float(0.5f) & ~constants::kFirstMask) + pair_offset;

      //Add modulation
      poly_float mod_depth = mod_depth_->buffer[0] * kMaxChorusModulation;
      poly_float mod = utils::sin(phase * vital::kPi * 2.0f) * 0.5f + 1.0f;

      //Calculate linearly increasing delay time out of 1
      float delay_t = 0.0f;

      if (i > 0)
        delay_t = i / (num_voices - 1.0f);
      
      //Calculate the final delay
      poly_float delay = mod * mod_depth + utils::interpolate(delay_time, average_delay, delay_t);

      //Calculates the frequency of the delay as the reciprocal of the delay time while avoiding division by zero
      vital::poly_float delay_frequency = poly_float(1.0f) / utils::max(0.00001f, delay);

      //Apply delay frequency
      delay_frequencies_[i].set(delay_frequency);
      
      //Process the delay
      delays_[i]->processWithInput(audio_out, num_samples);
      

      delay_status_outputs_[i].buffer[0] = delay_frequency;
    }

    //Get current wet and dry values
    poly_float current_wet = wet_;
    poly_float current_dry = dry_;

    poly_float wet_value = utils::clamp(wet_output_->buffer[0], 0.0f, 1.0f);

    //Calculates the dry and wet values from the mix to maintain a constant power
    wet_ = futils::equalPowerFade(wet_value);
    dry_ = futils::equalPowerFadeInverse(wet_value);

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_wet = (wet_ - current_wet) * tick_increment;
    poly_float delta_dry = (dry_ - current_dry) * tick_increment;

    //Clears the audio buffer
    utils::zeroBuffer(audio_out, num_samples);

    // Mix the delayed signals
    for (int i = 0; i < num_voices; ++i) {
      //Get the output buffer for each delay
      const poly_float* delay_out = delays_[i]->output()->buffer;

      for (int s = 0; s < num_samples; ++s) {
        //Apply half gain to the sample
        poly_float sample_out = delay_out[s] * 0.5f;

        //Sum and mix into the current output
        audio_out[s] += sample_out + utils::swapVoices(sample_out);
      }
    }

    // Apply wet/dry mix
    for (int s = 0; s < num_samples; ++s) {
      current_dry += delta_dry;
      current_wet += delta_wet;
      audio_out[s] = current_dry * audio_in[s] + current_wet * audio_out[s];
    }
  }

  // Correct the phase to match the given time in seconds
  void ChorusModule::correctToTime(double seconds) {
    phase_ = utils::getCycleOffsetFromSeconds(seconds, frequency_->buffer[0]);
  }
} // namespace vital
