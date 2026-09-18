#pragma once

#include "esphome/components/sen5x/sen5x.h"
#include "esphome/core/application.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <array>

namespace esphome::sen5x_mode {

// SEN5x I2C commands, from the Sensirion SEN5x datasheet. Re-declared here
// because the stock sen5x component keeps them file-local in sen5x.cpp.
static const uint16_t SEN5X_CMD_START_MEASUREMENT = 0x0021;
static const uint16_t SEN5X_CMD_START_MEASUREMENT_GAS_ONLY = 0x0037;
static const uint16_t SEN5X_CMD_STOP_MEASUREMENT = 0x3F86;
static const uint16_t SEN5X_CMD_VOC_ALGORITHM_STATE = 0x6181;

// The datasheet requires 200ms after stop_measurement before the sensor
// accepts another command, and 20ms between ordinary command/read pairs.
static const uint32_t SEN5X_STOP_DELAY_MS = 200;
static const uint32_t SEN5X_CMD_DELAY_MS = 20;

// The VOC/NOx algorithm state is 4 words.
static const uint8_t SEN5X_VOC_STATE_WORDS = 4;

enum MeasurementMode : uint16_t {
  // Fan and laser running: particulates plus temperature, humidity, VOC, NOx.
  MODE_FULL = SEN5X_CMD_START_MEASUREMENT,
  // Fan and laser off: temperature, humidity, VOC and NOx only. Particulate
  // registers read back as 0xFFFF, which the stock component publishes as NaN,
  // so PM sensors need a `filter_out: nan` to hold their last value.
  MODE_GAS_ONLY = SEN5X_CMD_START_MEASUREMENT_GAS_ONLY,
};

/* Switches a SEN5x between full and gas-only measurement at runtime.
 *
 * The stock sen5x component picks one mode in setup() and never changes it, so
 * the fan and laser run continuously for the life of the device. Parking the
 * sensor in gas-only mode and only spinning up for particulate samples cuts the
 * duty cycle on those wear items by an order of magnitude.
 *
 * Mode changes are only accepted in idle, so the sequence is stop -> wait ->
 * start. Stopping resets the VOC and NOx index algorithms, which would leave
 * both permanently uncalibrated at any useful sampling interval, so the
 * algorithm state is read out beforehand and written back while idle.
 */
template<typename... Ts> class SetModeAction final : public Action<Ts...> {
 public:
  explicit SetModeAction(sen5x::SEN5XComponent *parent) : parent_(parent) {}

  void set_mode(MeasurementMode mode) { this->mode_ = mode; }

  void play(const Ts &...x) override {
    auto *parent = this->parent_;
    const uint16_t mode = this->mode_;

    // Save the VOC/NOx algorithm state. Only readable while measuring.
    std::array<uint16_t, SEN5X_VOC_STATE_WORDS> voc_state{};
    bool have_voc_state = parent->write_command(SEN5X_CMD_VOC_ALGORITHM_STATE);
    if (have_voc_state) {
      delay(SEN5X_CMD_DELAY_MS);
      have_voc_state = parent->read_data(voc_state.data(), SEN5X_VOC_STATE_WORDS);
    }
    if (!have_voc_state) {
      ESP_LOGW(TAG, "Could not read VOC algorithm state; VOC/NOx index will restart");
    }

    if (!parent->write_command(SEN5X_CMD_STOP_MEASUREMENT)) {
      ESP_LOGW(TAG, "Failed to stop measurement; mode unchanged");
      return;
    }

    // Deferred rather than delay()ed so the 200ms settling time does not block
    // the main loop. Keyed on `this`, so a second switch supersedes a pending
    // one instead of interleaving with it.
    App.scheduler.set_timeout(this, SEN5X_STOP_DELAY_MS, [parent, mode, voc_state, have_voc_state]() {
      if (have_voc_state) {
        if (!parent->write_command(SEN5X_CMD_VOC_ALGORITHM_STATE, voc_state.data(), SEN5X_VOC_STATE_WORDS)) {
          ESP_LOGW(TAG, "Failed to restore VOC algorithm state");
        }
        delay(SEN5X_CMD_DELAY_MS);
      }

      if (!parent->write_command(mode)) {
        ESP_LOGE(TAG, "Failed to start measurement in mode 0x%04X; sensor left idle", mode);
        return;
      }
      ESP_LOGD(TAG, "Measurement mode set to %s", mode == SEN5X_CMD_START_MEASUREMENT ? "full" : "gas-only");
    });
  }

 protected:
  static constexpr const char *const TAG = "sen5x_mode";

  sen5x::SEN5XComponent *parent_;
  MeasurementMode mode_{MODE_FULL};
};

}  // namespace esphome::sen5x_mode
