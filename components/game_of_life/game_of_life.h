#pragma once
#include "esphome/core/component.h"
#include "esphome/components/display/display.h"
#include "esphome/core/color.h"
#include "esphome/core/preferences.h"

#define AGE_DEAD 0
#define AGE_1 1
#define AGE_2 2
#define AGE_n 3

#define random(minimum_number, max_number)  rand() % (max_number + 1) + minimum_number

namespace esphome {
namespace game_of_life {

// expose a number to set speed to enable/disable
class GameOfLife;
namespace game_of_life_number
{
    class GameOfLifeNumber;
    static void set_reference(GameOfLifeNumber *number_, GameOfLife *componenet);
} // 

const auto color_off = esphome::Color(0,0,0);
const auto color_age_1 = esphome::Color(0,128,0);
const auto color_age_2 = esphome::Color(128,0,0); 
const auto color_age_n = esphome::Color(0,0,128);

class GameOfLife : public Component {
    public:
        GameOfLife();
        void dump_config() override;
        void setup() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_size(int, int);
        void set_starting_density(int);
        void set_color_off(esphome::Color);
        void set_color_age_1(esphome::Color);
        void set_color_age_2(esphome::Color);
        void set_color_age_n(esphome::Color);
        void set_spark(bool spark);
        void set_speed(uint8_t);
        
        uint get_population();
        uint get_top_population();
        uint get_iteration();
        uint8_t get_speed();

        void spark_of_life();
        void reset();
        void draw(display::Display&, int, int);
        void start() { this->running_ = true; };
        void stop() { this->running_ = false; };

        std::vector<game_of_life_number::GameOfLifeNumber *> get_speed_numbers()
        {
            return numbers_;
        }

        void register_speed_number(game_of_life_number::GameOfLifeNumber *component_number)
        {
            numbers_.push_back(component_number);
            set_reference(component_number, this);
        };

    protected:
        void nextIteration();
        int64_t get_time_ns_();
        void set_next_call_ns_();
        uint8_t speed_;
        esphome::Color color_off, color_age_1, color_age_2, color_age_n;
        uint starting_density_;
        std::vector<std::vector<char>> current_state_;
        int rows, cols;
        uint iteration_;
        uint alive_, alive_same_count_, topWeight_;
        esphome::Mutex mutex_;
        bool run_spark_;
        bool running_ = false;
        int64_t next_call_ns_{0};
        int64_t last_time_ms_{0};
        uint32_t millis_overflow_counter_{0};
        std::vector<game_of_life_number::GameOfLifeNumber *> numbers_;
};

} // namespace game_of_life
} // namespace esphome
