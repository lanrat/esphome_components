#pragma once
#include "esphome/core/component.h"
#include "esphome/components/display/display.h"
#include "esphome/core/color.h"
#include "esphome/core/helpers.h"


#define AGE_DEAD 0
#define AGE_1 1
#define AGE_2 2
#define AGE_n 3

#define random(minimum_number, max_number)  rand() % (max_number + 1) + minimum_number

namespace esphome {
namespace game_of_life {

//const auto color_on = esphome::Color(255,255,255);
const auto color_off = esphome::Color(0,0,0);
// const auto color_age_1 = esphome::Color(0,128,0); // COLOR_CSS_GREEN
// const auto color_age_2 = esphome::Color(205,92,92); // COLOR_CSS_INDIANRED
// const auto color_age_n = esphome::Color(155,140,0); // COLOR_CSS_DARKORANGE

const auto color_age_1 = esphome::Color(0,128,0);
const auto color_age_2 = esphome::Color(128,0,0); 
const auto color_age_n = esphome::Color(0,0,128);

class GameOFLife : public Component {
    public:
        GameOFLife();
        void dump_config() override;
        void setup() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_display(display::Display *);
        void set_starting_density(int);
        void set_color_off(esphome::Color);
        void set_color_age_1(esphome::Color);
        void set_color_age_2(esphome::Color);
        void set_color_age_n(esphome::Color);
        void set_spark(bool spark);
        
        uint get_population();
        uint get_top_population();
        uint get_iteration();
        
        void spark_of_life();
        void reset();
        void render();

    protected:
        void nextIteration();
        esphome::Color color_off, color_age_1, color_age_2, color_age_n;
        uint starting_density_;
        std::vector<std::vector<char>> current_state_;
        display::Display * display_;
        int rows, cols;
        uint iteration_;
        uint alive_, alive_same_count_, topWeight_;
        esphome::Mutex mutex_;
        bool run_spark_;
};

} // namespace game_of_life
} // namespace esphome
