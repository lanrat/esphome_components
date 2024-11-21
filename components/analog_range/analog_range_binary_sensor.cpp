#include "analog_range_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace analog_range {

static const char *const TAG = "analog_range.binary_sensor";

void AnalogRangeBinarySensor::setup() {
  float sensor_value = this->sensor_->get_state();

  // TRUE state is defined to be when sensor is >= lower_range_ and <= upper_range_
  // so when undefined sensor value initialize to FALSE
  if (std::isnan(sensor_value)) {
    this->publish_initial_state(false);
  } else {
    this->publish_initial_state((sensor_value >= this->lower_range_) && (sensor_value <= this->upper_range_));
  }
}

void AnalogRangeBinarySensor::set_sensor(sensor::Sensor *analog_sensor) {
  this->sensor_ = analog_sensor;

  this->sensor_->add_on_state_callback([this](float sensor_value) {
    // if there is an invalid sensor reading, ignore the change and keep the current state
    if (!std::isnan(sensor_value)) {
      //this->publish_state(sensor_value >= (this->state ? this->lower_range_ : this->upper_range_));
      this->publish_state((sensor_value >= this->lower_range_) && (sensor_value <= this->upper_range_));
    }
  });
}

void AnalogRangeBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "Analog Range Binary Sensor", this);
  LOG_SENSOR("  ", "Sensor", this->sensor_);
  ESP_LOGCONFIG(TAG, "  Upper range: %.11f", this->upper_range_);
  ESP_LOGCONFIG(TAG, "  Lower range: %.11f", this->lower_range_);
}

}  // namespace analog_range
}  // namespace esphome