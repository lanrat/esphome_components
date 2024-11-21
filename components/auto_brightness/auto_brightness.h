#pragma once
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"
#include <map>

namespace esphome {
namespace auto_brightness {

// expose a switch to enable/disable
class AutoBrightness;
namespace auto_brightness_switch
{
    class AutoBrightnessSwitch;
    static void set_reference(AutoBrightnessSwitch *switch_, AutoBrightness *componenet);
} // 

class AutoBrightness : public Component {
    public:
        void dump_config() override;
        void setup() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_sensor(sensor::Sensor *);
        void set_number(number::Number *);
        void set_levels(std::vector<std::array<float, 2>>);
        void set_decrease_offset(float);

        void set_state(bool state);

        std::vector<auto_brightness_switch::AutoBrightnessSwitch *> get_enable_switches()
        {
            return switches_;
        }

        void register_enable_switch(auto_brightness_switch::AutoBrightnessSwitch *component_switch)
        {
            switches_.push_back(component_switch);
            set_reference(component_switch, this);
        };

    protected:
        bool enabled_ = true;
        std::map<float, float> levels;
        number::Number *number_{nullptr};
        sensor::Sensor *sensor_{nullptr};
        bool rising;
        float decrease_light_level_offset;
        std::vector<auto_brightness_switch::AutoBrightnessSwitch *> switches_;

        void update_brightness(float);
};

} // namespace auto_brightness
} // namespace esphome
