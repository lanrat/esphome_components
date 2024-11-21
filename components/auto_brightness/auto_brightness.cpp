#include "auto_brightness.h"
#include "esphome/core/log.h"

namespace esphome {
namespace auto_brightness {

static const char *const TAG = "auto_brightness";


void AutoBrightness::setup() {
    this-> rising = true;
}

void AutoBrightness::set_sensor(sensor::Sensor *sensor) {
  this->sensor_ = sensor;

  this->sensor_->add_on_state_callback([this](float sensor_value) {
    // if there is an invalid sensor reading, ignore the change and keep the current state
    if (!std::isnan(sensor_value) && this->number_!= nullptr) {
      this->update_brightness(sensor_value);
    }
  });
}

void AutoBrightness::set_number(number::Number *number) {
  this->number_ = number;
}

void AutoBrightness::set_decrease_offset(float offset) {
  this->decrease_light_level_offset = offset;
}

void AutoBrightness::set_levels(std::vector<std::array<float, 2>> levels) {
    // convert vector or array to map
    for (const auto level : levels) {
        this->levels[level[0]] = level[1];
    }
}

void AutoBrightness::update_brightness(float sensor_value) {
   // adjust the offset if we are decreasing
    float offset = 0;
    if (!this->rising) {
        offset = this->decrease_light_level_offset;
    }

    // find the first matching brightness level in the map
    auto level = this->levels.lower_bound(sensor_value-offset);

    // if not found, decrease iterator to be the highest
    if (level == this->levels.end()) {
        // brightness larger than max value, use highest value.
        level--;
    }

    float new_brightness = level->second;

    // set rising and update brightness level if changing
    if (new_brightness != sensor_value) {
    //if (new_brightness != matrix->get_initial_brightness()) {
        rising = (new_brightness > this->number_->state);
        ESP_LOGD(__func__, "light level: %.5f brightness: %.5f, rising: %d, offset: %.5f", sensor_value, new_brightness, rising, offset);
        this->number_->make_call().set_value(new_brightness).perform();
    }
}

void AutoBrightness::dump_config() {
  LOG_SENSOR("  ", "Sensor", this->sensor_);
  LOG_NUMBER("  ", "Number", this->number_);
  ESP_LOGCONFIG(TAG, "Decrease Offset", this->decrease_light_level_offset);
  for (auto &level : this->levels) {
    ESP_LOGCONFIG(TAG, "  Level: %d -> %d", level.first, level.second);
  }
}

} // namespace auto_brightness
} // namespace esphome
