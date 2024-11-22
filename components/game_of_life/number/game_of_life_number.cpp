#include "game_of_life_number.h"

namespace esphome::game_of_life::game_of_life_number
{
    void GameOfLifeNumber::control(float value)
    {
        // Update component status and forward state to all registered switches.
        this->componenet_->set_speed(value);
        for (GameOfLifeNumber *number_ : this->componenet_->get_speed_numbers())
        {
            number_->publish_state(value);
        }
    };

    void GameOfLifeNumber::dump_config()
    {
        ESP_LOGCONFIG(TAG, "GameOfLife Number");
    };
} // namespace esphome::game_of_life::game_of_life_number