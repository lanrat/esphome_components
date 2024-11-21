#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace analog_range {

class AnalogRangeBinarySensor : public Component, public binary_sensor::BinarySensor {
 public:
  void dump_config() override;
  void setup() override;

  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_sensor(sensor::Sensor *analog_sensor);
  void set_upper_range(float range) { this->upper_range_ = range; }
  void set_lower_range(float range) { this->lower_range_ = range; }

 protected:
  sensor::Sensor *sensor_{nullptr};

  float upper_range_;
  float lower_range_;
};

}  // namespace analog_range
}  // namespace esphome