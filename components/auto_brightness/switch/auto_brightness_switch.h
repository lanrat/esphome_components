#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../auto_brightness.h"

namespace esphome::auto_brightness::auto_brightness_switch
{
    static const char *TAG = "AutoBrightnessSwitch";

    class AutoBrightnessSwitch : public switch_::Switch, public Component
    {
    public:
        /**
         * Write the requested user state to hardware.
         */
        void write_state(bool state);

        void setup() override;
        void dump_config() override;

        /**
         * Sets the reference to the component to which this switch belongs.
         *
         * @param componenet component reference
         */
        void set_controller(AutoBrightness *componenet)
        {
            this->componenet_ = componenet;
        }

    protected:
        /// @brief component to which this switch belongs
        AutoBrightness *componenet_;
    };

    /**
     * Sets the reference to the component to which this switch belongs.
     *
     * @param switch_ switch for which the reference should be set
     * @param componenet component reference
     */
    static void set_reference(AutoBrightnessSwitch *switch_, AutoBrightness *componenet)
    {
        switch_->set_controller(componenet);
    }
} // namespace esphome::auto_brightness::auto_brightness_switch