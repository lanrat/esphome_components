#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../game_of_life.h"

namespace esphome::game_of_life::game_of_life_number
{
    static const char *TAG = "GameOfLifeNumber";

    class GameOfLifeNumber : public number::Number, public Component
    {
    public:
        /**
         * Write the requested user state to hardware.
         */
        void write_state(bool state);

        void setup()
        {
            publish_state(this->componenet_->get_speed());
        }

        void control(float value);

        void dump_config() override;

        /**
         * Sets the reference to the component to which this number belongs.
         *
         * @param componenet component reference
         */
        void set_controller(GameOfLife *componenet)
        {
            this->componenet_ = componenet;
        }

    protected:
        /// @brief component to which this number belongs
        GameOfLife *componenet_;
    };

    /**
     * Sets the reference to the component to which this number belongs.
     *
     * @param number number for which the reference should be set
     * @param componenet component reference
     */
    static void set_reference(GameOfLifeNumber *number_, GameOfLife *componenet)
    {
        number_->set_controller(componenet);
    }

    static void publish_state(GameOfLifeNumber *number, int value)
    {
        number->publish_state(value);
    }
} // namespace esphome::game_of_life::game_of_life_number
