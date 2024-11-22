#include "scrolling_text.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace scrolling_text {

static const char *const TAG = "scrolling_text";

bool ScrollingText::running() {
  return this->running_;
}

void ScrollingText::stop() {
    this->running_ = false;
    this->scroll_ = false;
}

// TODO not sure this works right
bool ScrollingText::stop_when_clear() {
  // if text is not scrolling
  if (!this->scroll_) {
    this->stop();
    return true;
  }
  if (this->scroll_count_ >= 1 && this->text_x_ <= 1) {
      this->stop();
      return true;
  }
  return false;
}

void ScrollingText::print(int width, int height,
            display::BaseFont* font, esphome::Color color,
            uint32_t frame_delay_ms,
            bool center_short,
            std::string text) {
  this->print(width, height, font, color, frame_delay_ms, center_short, text.c_str());
}

void ScrollingText::print(int width, int height,
            display::BaseFont* font, esphome::Color color,
            uint32_t frame_delay_ms,
            bool center_short,
            const char * text) {
  this->printf(width, height, font, color, frame_delay_ms, center_short, "%s", text);
}

void ScrollingText::printf(int width, int height,
            display::BaseFont* font, esphome::Color color,
            uint32_t frame_delay_ms,
            bool center_short,
            const char *format, ...) {
  
  va_list arg;
  va_start(arg, format);
  int ret = vsnprintf(this->text_, sizeof(this->text_), format, arg);
  va_end(arg);
  if (ret <= 0) {
    ESP_LOGE(TAG, "snprintf buf(%d) returned %d for format: %s", ret, sizeof(this->text_), format);
    return;
  }
  
  this->width_ = width;
  this->height_ = height;
  this->font_ = font;
  this->color_ = color;
  this->frame_delay_ms_ = frame_delay_ms;
  this->text_width_ = get_text_width_(this->font_, this->text_);

  // if text will not scroll, center it
  if (center_short && this->text_width_ <= this->width_) {
    this->text_x_ = (this->width_ - this->text_width_) / 2;
    // set scroll count to 1 to indicate that the entire message has been seen
    this->scroll_count_ = 1;
    this->scroll_ = false;
  } else {
    // else text starts at right end
    this->text_x_ = this->width_;
    // reset scroll counter
    this->scroll_count_ = 0;
    this->scroll_ = true;

    ESP_LOGE(TAG, "screen: %dx%d text width: %d, starting_x: %d", this->width_, this->height_, this->text_width_, this->text_x_);
  }

  // start
  this->running_ = true;
}

void ScrollingText::draw(display::Display& display, int x, int y) {
  // render the text on the display
  if (this->running_ && this->font_ != NULL) {
    int x_pos = x + this->text_x_;
    int y_pos = y + (this->height_/2);
    //ESP_LOGD(TAG, "displaying text at: (%d,%d) color: %d %s", x_pos, y_pos, this->color_.raw_32, this->text_);
    display.printf(x_pos, y_pos, this->font_, this->color_, display::TextAlign::CENTER_LEFT, "%s", this->text_);
  }
}

void ScrollingText::loop() {
  if (!this->running_) return;

  // check if this should run based on speed
  int64_t curr_time_ns = this->get_time_ns_();
  if (curr_time_ns < this->next_call_ns_) {
    return;
  }
  
  // set next time to run
  this->set_next_call_ns_();

  // loop the text
  if (this->running_ && this->scroll_) {
    this->text_x_--;

    // test if resetting scroll back to right of screen
    if (this->text_width_ + this->text_x_ <= 0) {
      ESP_LOGD(TAG, "text off screen, resetting scrolling_text");
      // scrolled off screen, reset
      this->text_x_ = this->width_;
      // increase scroll count
      this->scroll_count_++;
    }
  }
}

void ScrollingText::set_next_call_ns_() {
  auto wait_ms = this->frame_delay_ms_;
  //ESP_LOGD(TAG, "Waiting for %d ms for next iteration", wait_ms);
  this->next_call_ns_ = (wait_ms * INT64_C(1000000)) + this->get_time_ns_();
}

int64_t ScrollingText::get_time_ns_() {
  int64_t time_ms = millis();
  if (this->last_time_ms_ > time_ms) {
    this->millis_overflow_counter_++;
  }
  this->last_time_ms_ = time_ms;
  return (time_ms + ((int64_t) this->millis_overflow_counter_ << 32)) * INT64_C(1000000);
}

void ScrollingText::dump_config() {
  ESP_LOGCONFIG(TAG, "ScrollingText");
}

// Get the width (in pixels) of a char string using a given Font
int ScrollingText::get_text_width_(display::BaseFont *font, const char *text) {
    int width, x_offset, baseline, height;
    font->measure(text, &width, &x_offset, &baseline, &height);
    return width;
};

} // namespace scrolling_text
} // namespace esphome
