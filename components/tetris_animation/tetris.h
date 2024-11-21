#pragma once
#include "esphome/components/display/display.h"
#include "esphome/components/time/real_time_clock.h"
#include "TetrisMatrixDraw.h"

// TODO adjust speed
// TODO force reset

namespace esphome {
namespace tetris_animation {

class TetrisAnimation : public Component {
    public:
        void dump_config() override;
        void setup() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        void set_display(display::Display *);
        void set_scale(int);
        void set_time_source(time::RealTimeClock *);

        void draw();
    
    protected:
        void updateTime();
        TetrisMatrixDraw tetris_;
        time::RealTimeClock *rtc_;
        char time_buffer_[10];
        uint8_t last_hour_, last_min_;
};

} // namespace tetris_animation
} // namespace esphome
