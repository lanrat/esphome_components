

#include "bme680_bsec_spi.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <string>

namespace esphome {
namespace bme680_bsec_spi {
#ifdef USING_BSEC
static const char *TAG = "bme680_bsec_spi.sensor";

static const std::string IAQ_ACCURACY_STATES[4] = {"Stabilizing", "Uncertain", "Calibrating", "Calibrated"};

BME680BSECComponent *BME680BSECComponent::instance;

void BME680BSECComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BME680 via BSEC SPI...");
  this->spi_setup();
  BME680BSECComponent::instance = this;

  this->bsec_status_ = bsec_init();
  if (this->bsec_status_ != BSEC_OK) {
    this->mark_failed();
    return;
  }

  this->bme680_.dev_id = 0;
  this->bme680_.intf = BME680_SPI_INTF;
  this->bme680_.read = BME680BSECComponent::read_bytes_wrapper;
  this->bme680_.write = BME680BSECComponent::write_bytes_wrapper;
  this->bme680_.delay_ms = BME680BSECComponent::delay_ms;
  this->bme680_.amb_temp = 25;
  this->bme680_.power_mode = BME680_FORCED_MODE;

  this->bme680_status_ = bme680_init(&this->bme680_);
  if (this->bme680_status_ != BME680_OK) {
    this->mark_failed();
    return;
  }

  if (this->sample_rate_ == SAMPLE_RATE_ULP) {
    const uint8_t bsec_config[] = {
#include "config/generic_33v_300s_28d/bsec_iaq.txt"
    };
    this->set_config_(bsec_config);
    this->update_subscription_(BSEC_SAMPLE_RATE_ULP);
  } else {
    const uint8_t bsec_config[] = {
#include "config/generic_33v_3s_28d/bsec_iaq.txt"
    };
    this->set_config_(bsec_config);
    this->update_subscription_(BSEC_SAMPLE_RATE_LP);
  }
  if (this->bsec_status_ != BSEC_OK) {
    this->mark_failed();
    return;
  }

  this->load_state_();
}

void BME680BSECComponent::set_config_(const uint8_t *config) {
  uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
  this->bsec_status_ = bsec_set_configuration(config, BSEC_MAX_PROPERTY_BLOB_SIZE, work_buffer, sizeof(work_buffer));
}

void BME680BSECComponent::update_subscription_(float sample_rate) {
  bsec_sensor_configuration_t virtual_sensors[BSEC_NUMBER_OUTPUTS];
  int num_virtual_sensors = 0;

  if (this->iaq_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id =
        this->iaq_mode_ == IAQ_MODE_STATIC ? BSEC_OUTPUT_STATIC_IAQ : BSEC_OUTPUT_IAQ;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->co2_equivalent_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_CO2_EQUIVALENT;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->breath_voc_equivalent_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->pressure_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->gas_resistance_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_RAW_GAS;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->temperature_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  if (this->humidity_sensor_) {
    virtual_sensors[num_virtual_sensors].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    virtual_sensors[num_virtual_sensors].sample_rate = sample_rate;
    num_virtual_sensors++;
  }

  bsec_sensor_configuration_t sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
  uint8_t num_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
  this->bsec_status_ =
      bsec_update_subscription(virtual_sensors, num_virtual_sensors, sensor_settings, &num_sensor_settings);
}

void BME680BSECComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "BME680 via BSEC_SPI:");

  bsec_version_t version;
  bsec_get_version(&version);
  ESP_LOGCONFIG(TAG, "  BSEC Version: %d.%d.%d.%d", version.major, version.minor, version.major_bugfix,
                version.minor_bugfix);

  LOG_PIN("  CS Pin: ", this->cs_);

  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication failed (BSEC Status: %d, BME680 Status: %d)", this->bsec_status_,
             this->bme680_status_);
  }

  ESP_LOGCONFIG(TAG, "  Temperature Offset: %.2f", this->temperature_offset_);
  ESP_LOGCONFIG(TAG, "  IAQ Mode: %s", this->iaq_mode_ == IAQ_MODE_STATIC ? "Static" : "Mobile");
  ESP_LOGCONFIG(TAG, "  Sample Rate: %s", this->sample_rate_ == SAMPLE_RATE_ULP ? "ULP" : "LP");
  ESP_LOGCONFIG(TAG, "  State Save Interval: %ims", this->state_save_interval_ms_);

  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Pressure", this->pressure_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "Gas Resistance", this->gas_resistance_sensor_);
  LOG_SENSOR("  ", "IAQ", this->iaq_sensor_);
  LOG_SENSOR("  ", "Numeric IAQ Accuracy", this->iaq_accuracy_sensor_);
  LOG_TEXT_SENSOR("  ", "IAQ Accuracy", this->iaq_accuracy_text_sensor_);
  LOG_SENSOR("  ", "CO2 Equivalent", this->co2_equivalent_sensor_);
  LOG_SENSOR("  ", "Breath VOC Equivalent", this->breath_voc_equivalent_sensor_);
}

float BME680BSECComponent::get_setup_priority() const { return setup_priority::DATA; }

void BME680BSECComponent::loop() {
  this->run_();

  if (this->bsec_status_ < BSEC_OK || this->bme680_status_ < BME680_OK) {
    this->status_set_error();
  } else {
    this->status_clear_error();
  }
  if (this->bsec_status_ > BSEC_OK || this->bme680_status_ > BME680_OK) {
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
}

void BME680BSECComponent::run_() {
  int64_t curr_time_ns = this->get_time_ns_();
  if (curr_time_ns < this->next_call_ns_) {
    return;
  }

  ESP_LOGV(TAG, "Performing sensor run");

  bsec_bme_settings_t bme680_settings;
  this->bsec_status_ = bsec_sensor_control(curr_time_ns, &bme680_settings);
  if (this->bsec_status_ < BSEC_OK) {
    ESP_LOGW(TAG, "Failed to fetch sensor control settings (BSEC Error Code %d)", this->bsec_status_);
    return;
  }
  this->next_call_ns_ = bme680_settings.next_call;

  this->bme680_.gas_sett.run_gas = bme680_settings.run_gas;
  this->bme680_.tph_sett.os_hum = bme680_settings.humidity_oversampling;
  this->bme680_.tph_sett.os_temp = bme680_settings.temperature_oversampling;
  this->bme680_.tph_sett.os_pres = bme680_settings.pressure_oversampling;
  this->bme680_.gas_sett.heatr_temp = bme680_settings.heater_temperature;
  this->bme680_.gas_sett.heatr_dur = bme680_settings.heating_duration;
  uint16_t desired_settings =
      BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;
  this->bme680_status_ = bme680_set_sensor_settings(desired_settings, &this->bme680_);
  if (this->bme680_status_ != BME680_OK) {
    ESP_LOGW(TAG, "Failed to set sensor settings (BME680 Error Code %d)", this->bme680_status_);
    return;
  }

  this->bme680_status_ = bme680_set_sensor_mode(&this->bme680_);
  if (this->bme680_status_ != BME680_OK) {
    ESP_LOGW(TAG, "Failed to set sensor mode (BME680 Error Code %d)", this->bme680_status_);
    return;
  }

  uint16_t meas_dur = 0;
  bme680_get_profile_dur(&meas_dur, &this->bme680_);
  ESP_LOGV(TAG, "Queueing read in %ums", meas_dur);
  this->set_timeout("read", meas_dur, [this, bme680_settings]() { this->read_(bme680_settings); });
}

void BME680BSECComponent::read_(bsec_bme_settings_t bme680_settings) {
  ESP_LOGV(TAG, "Reading data");
  struct bme680_field_data data;
  this->bme680_status_ = bme680_get_sensor_data(&data, &this->bme680_);

  if (this->bme680_status_ != BME680_OK) {
    ESP_LOGW(TAG, "Failed to get sensor data (BME680 Error Code %d)", this->bme680_status_);
    return;
  }
  if (!(data.status & BME680_NEW_DATA_MSK)) {
    ESP_LOGD(TAG, "BME680 did not report new data");
    return;
  }

  bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];  // Temperature, Pressure, Humidity & Gas Resistance
  uint8_t num_inputs = 0;
  int64_t curr_time_ns = this->get_time_ns_();

  if (bme680_settings.process_data & BSEC_PROCESS_TEMPERATURE) {
    inputs[num_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
    inputs[num_inputs].signal = data.temperature / 100.0f;
    inputs[num_inputs].time_stamp = curr_time_ns;
    num_inputs++;

    // Temperature offset from the real temperature due to external heat sources
    inputs[num_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
    inputs[num_inputs].signal = this->temperature_offset_;
    inputs[num_inputs].time_stamp = curr_time_ns;
    num_inputs++;
  }
  if (bme680_settings.process_data & BSEC_PROCESS_HUMIDITY) {
    inputs[num_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
    inputs[num_inputs].signal = data.humidity / 1000.0f;
    inputs[num_inputs].time_stamp = curr_time_ns;
    num_inputs++;
  }
  if (bme680_settings.process_data & BSEC_PROCESS_PRESSURE) {
    inputs[num_inputs].sensor_id = BSEC_INPUT_PRESSURE;
    inputs[num_inputs].signal = data.pressure;
    inputs[num_inputs].time_stamp = curr_time_ns;
    num_inputs++;
  }
  if (bme680_settings.process_data & BSEC_PROCESS_GAS) {
    inputs[num_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
    inputs[num_inputs].signal = data.gas_resistance;
    inputs[num_inputs].time_stamp = curr_time_ns;
    num_inputs++;
  }
  if (num_inputs < 1) {
    ESP_LOGD(TAG, "No signal inputs available for BSEC");
    return;
  }

  bsec_output_t outputs[BSEC_NUMBER_OUTPUTS];
  uint8_t num_outputs = BSEC_NUMBER_OUTPUTS;
  this->bsec_status_ = bsec_do_steps(inputs, num_inputs, outputs, &num_outputs);
  if (this->bsec_status_ != BSEC_OK) {
    ESP_LOGW(TAG, "BSEC failed to process signals (BSEC Error Code %d)", this->bsec_status_);
    return;
  }
  if (num_outputs < 1) {
    ESP_LOGD(TAG, "No signal outputs provided by BSEC");
    return;
  }

  this->publish_(outputs, num_outputs);
}

void BME680BSECComponent::publish_(const bsec_output_t *outputs, uint8_t num_outputs) {
  ESP_LOGV(TAG, "Publishing sensor states");
  for (uint8_t i = 0; i < num_outputs; i++) {
    switch (outputs[i].sensor_id) {
      case BSEC_OUTPUT_IAQ:
      case BSEC_OUTPUT_STATIC_IAQ:
        uint8_t accuracy;
        accuracy = outputs[i].accuracy;
        this->publish_sensor_state_(this->iaq_sensor_, outputs[i].signal);
        this->publish_sensor_state_(this->iaq_accuracy_text_sensor_, IAQ_ACCURACY_STATES[accuracy]);
        this->publish_sensor_state_(this->iaq_accuracy_sensor_, accuracy, true);

        // Queue up an opportunity to save state
        this->defer("save_state", [this, accuracy]() { this->save_state_(accuracy); });
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        this->publish_sensor_state_(this->co2_equivalent_sensor_, outputs[i].signal);
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        this->publish_sensor_state_(this->breath_voc_equivalent_sensor_, outputs[i].signal);
        break;
      case BSEC_OUTPUT_RAW_PRESSURE:
        this->publish_sensor_state_(this->pressure_sensor_, outputs[i].signal / 100.0f);
        break;
      case BSEC_OUTPUT_RAW_GAS:
        this->publish_sensor_state_(this->gas_resistance_sensor_, outputs[i].signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        this->publish_sensor_state_(this->temperature_sensor_, outputs[i].signal);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        this->publish_sensor_state_(this->humidity_sensor_, outputs[i].signal);
        break;
    }
  }
}

int64_t BME680BSECComponent::get_time_ns_() {
  int64_t time_ms = millis();
  if (this->last_time_ms_ > time_ms) {
    this->millis_overflow_counter_++;
  }
  this->last_time_ms_ = time_ms;

  return (time_ms + ((int64_t) this->millis_overflow_counter_ << 32)) * INT64_C(1000000);
}

void BME680BSECComponent::publish_sensor_state_(sensor::Sensor *sensor, float value, bool change_only) {
  if (!sensor || (change_only && sensor->has_state() && sensor->state == value)) {
    return;
  }
  sensor->publish_state(value);
}

void BME680BSECComponent::publish_sensor_state_(text_sensor::TextSensor *sensor, std::string value) {
  if (!sensor || (sensor->has_state() && sensor->state == value)) {
    return;
  }
  sensor->publish_state(value);
}

int8_t BME680BSECComponent::read_bytes_wrapper(uint8_t address, uint8_t a_register, uint8_t *data, uint16_t len) {
  ESP_LOGVV(TAG, "read_bytes_wrapper(a:%02x r:%02x l:%d)", address, a_register, len);
  BME680BSECComponent::instance->enable();
  BME680BSECComponent::instance->write_byte(a_register);
  BME680BSECComponent::instance->read_array(data, len);
  BME680BSECComponent::instance->disable();
  return 0;
}

int8_t BME680BSECComponent::write_bytes_wrapper(uint8_t address, uint8_t a_register, uint8_t *data, uint16_t len) {
  ESP_LOGVV(TAG, "write_bytes_wrapper(a:%02x r:%02x l:%d)", address, a_register, len);
  BME680BSECComponent::instance->enable();
  BME680BSECComponent::instance->write_byte(a_register);
  BME680BSECComponent::instance->write_array(data, len);
  BME680BSECComponent::instance->disable();
  return 0;
}

void BME680BSECComponent::delay_ms(uint32_t period) {
  ESP_LOGV(TAG, "Delaying for %ums", period);
  delay(period);
}

void BME680BSECComponent::load_state_() {
  uint32_t hash = fnv1_hash("bme680_bsec_i2c_state_" + to_string(this->cs_->get_pin()));
  this->bsec_state_ = global_preferences.make_preference<uint8_t[BSEC_MAX_STATE_BLOB_SIZE]>(hash, true);

  uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
  if (this->bsec_state_.load(&state)) {
    ESP_LOGV(TAG, "Loading state");
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
    this->bsec_status_ = bsec_set_state(state, BSEC_MAX_STATE_BLOB_SIZE, work_buffer, sizeof(work_buffer));
    if (this->bsec_status_ != BSEC_OK) {
      ESP_LOGW(TAG, "Failed to load state (BSEC Error Code %d)", this->bsec_status_);
    }
    ESP_LOGI(TAG, "Loaded state");
  }
}

void BME680BSECComponent::save_state_(uint8_t accuracy) {
  if (accuracy < 3 || (millis() - this->last_state_save_ms_ < this->state_save_interval_ms_)) {
    return;
  }

  ESP_LOGV(TAG, "Saving state");

  uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
  uint8_t work_buffer[BSEC_MAX_STATE_BLOB_SIZE];
  uint32_t num_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;

  this->bsec_status_ =
      bsec_get_state(0, state, BSEC_MAX_STATE_BLOB_SIZE, work_buffer, BSEC_MAX_STATE_BLOB_SIZE, &num_serialized_state);
  if (this->bsec_status_ != BSEC_OK) {
    ESP_LOGW(TAG, "Failed fetch state for save (BSEC Error Code %d)", this->bsec_status_);
    return;
  }

  if (!this->bsec_state_.save(&state)) {
    ESP_LOGW(TAG, "Failed to save state");
    return;
  }
  this->last_state_save_ms_ = millis();

  ESP_LOGI(TAG, "Saved state");
}
#endif
}  // namespace bme680_bsec_spi
}  // namespace esphome
