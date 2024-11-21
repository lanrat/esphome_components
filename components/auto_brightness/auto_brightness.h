#pragma once
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include <map>

namespace esphome {
namespace auto_brightness {

class AutoBrightness : public Component {
    public:
        void dump_config() override;
        void setup() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_sensor(sensor::Sensor *);
        void set_number(number::Number *);
        void set_levels(std::vector<std::array<float, 2>>);
        void set_decrease_offset(float);

    protected:
        std::map<float, float> levels;
        number::Number *number_{nullptr};
        sensor::Sensor *sensor_{nullptr};
        bool rising;
        float decrease_light_level_offset;

        void update_brightness(float);
};

} // namespace auto_brightness
} // namespace esphome
