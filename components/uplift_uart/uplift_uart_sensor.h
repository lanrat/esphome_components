#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace uplift_uart {

class UpliftUartSensor : public sensor::Sensor, public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override {
    // Send a byte on TX to make the desk respond with its height
    this->write_byte(0);
  }

  void loop() override {
    while (this->available()) {
      uint8_t in;
      this->read_byte(&in);
      if (in > 10) {
        this->height_ = (25.6f * this->last_byte_) + (in / 10.0f);
      }
      this->last_byte_ = in;
    }
  }

  void update() override {
    if (this->height_ != this->last_height_) {
      this->publish_state(this->height_);
      this->last_height_ = this->height_;
    }
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  float height_ = -1.0f;
  float last_height_ = -1.0f;
  uint8_t last_byte_ = 0;
};

}  // namespace uplift_uart
}  // namespace esphome
