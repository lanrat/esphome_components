#pragma once
#include "esphome/components/display/display.h"
#include "esphome/components/time/real_time_clock.h"
#include "TetrisMatrixDraw.h"

// TODO adjust speed

namespace esphome {
namespace tetris_animation {

class TetrisAnimation : public Component {
    public:
        void dump_config() override;
        void setup() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_display(display::Display *);
        void set_scale(int);
        void set_time_source(time::RealTimeClock *);

        void draw();
    
    protected:
        void updateTime();
        TetrisMatrixDraw tetris;
        time::RealTimeClock *time_source;
        char time_buffer[10];
        uint8_t last_hour, last_min;
};

} // namespace tetris_animation
} // namespace esphome
