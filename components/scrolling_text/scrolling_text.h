#pragma once
#include "esphome/core/component.h"
#include "esphome/components/display/display.h"
#include "esphome/core/color.h"

namespace esphome {
namespace scrolling_text {

// TODO can support multiple scrolling at the same time if I use a vector of states.

class ScrollingText : public Component {
    public:
        void dump_config() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; }

        bool running();
        void stop();

        // attempts to stop the animation if at a reset point
        // if it succeeds, returns true, else false and try again.
        bool stop_when_clear();

        void set_frame_delay(uint32_t delay) { this->frame_delay_ms_ = delay; };
        int get_scroll_count() { return this->scroll_count_; };
        void set_dimensions(int width, int height) { this->width_ = width; this->height_ = height; };

        void render(display::Display& display, int x, int y);

        void printf(int width, int height,
            display::BaseFont* font, esphome::Color color,
            uint32_t frame_delay_ms,
            bool center_short,
            const char *format, ...);

    protected:
        bool running_ = false;
        display::BaseFont* font_;
        int width_, height_;
        bool scroll_;
        int text_width_;
        int text_x_;
        char text_[256];
        esphome::Color color_;
        int scroll_count_ = 0;
        uint32_t frame_delay_ms_;

        // timer
        int64_t get_time_ns_();
        void set_next_call_ns_();
        int64_t next_call_ns_{0};
        int64_t last_time_ms_{0};
        uint32_t millis_overflow_counter_{0};

        // utils
        int get_text_width_(display::BaseFont *font, const char *text);
};

} // namespace scrolling_text
} // namespace esphome
