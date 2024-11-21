#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome.h"

namespace esphome {
namespace zmpt101b {

class ZMPT101BSensor : public sensor::Sensor, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override {
    // After the base sensor has been initialized
    return setup_priority::DATA - 1.0f;
  }
  void set_source(voltage_sampler::VoltageSampler *source) { source_ = source; }
  void set_conf_calibration(float calibration) { this->calibration_ = calibration; }
  void set_conf_reference(float reference) { this->reference_ = reference; }

  void set_conf_number_of_samples(uint32_t number_of_samples) { number_of_samples_ = number_of_samples; }
  void set_conf_frequency(uint32_t frequency) { this->frequency_ = frequency; }

  double Vrms;

 protected:
  /// The sampling source to read values from.
  voltage_sampler::VoltageSampler *source_;

  float calibration_;
  float reference_;
  uint32_t number_of_samples_;
  uint32_t frequency_;
  float sample_sum_ = 0.0f;
  uint32_t num_samples_ = 0;
  bool is_sampling_ = false;

private:
  void calcV(unsigned int crossings, unsigned int timeout);
  float startV;                         // Instantaneous voltage at start of sample window.
  float lastFilteredV,filteredV;        // Filtered_ is the raw analog value minus the DC offset
  float sampleV;                        // sample_ holds the raw analog read value
  float offsetV;                        // Low-pass filter output
  float sqV,sumV;                       // sq = squared, sum = Sum, inst = instantaneous
  bool lastVCross, checkVCross;         // Used to measure number of times threshold is crossed.

  // for testing
  float min, max;
  float filteredMin, filteredMax;
};

}  // namespace zmpt101b
}  // namespace esphome
