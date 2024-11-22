#include "auto_brightness_switch.h"

namespace esphome::auto_brightness::auto_brightness_switch
{
    void AutoBrightnessSwitch::setup()
    {
        auto initial_state = this->get_initial_state_with_restore_mode().value_or(false);
        ESP_LOGD(TAG, "Setting up component switch with initial state: %i", initial_state);
        this->write_state(initial_state);
    }

    void AutoBrightnessSwitch::write_state(bool state)
    {
        // Update component status and forward state to all registered switches.
        this->componenet_->set_state(state);

        for (AutoBrightnessSwitch *switch_ : this->componenet_->get_enable_switches())
        {
            switch_->publish_state(state);
        }
    };

    void AutoBrightnessSwitch::dump_config()
    {
        ESP_LOGCONFIG(TAG, "AutoBrightness Switch");
    };
} // namespace esphome::auto_brightness::auto_brightness_switch