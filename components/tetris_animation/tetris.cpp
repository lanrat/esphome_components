#include "tetris.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tetris_animation {

static const char *const TAG = "tetris_animation";

void TetrisAnimation::setup() {
  this->reset();
}

void TetrisAnimation::set_display(display::Display *display) {
  this->tetris_.display = display;
}

void TetrisAnimation::set_time_source(time::RealTimeClock *time) {
  this->rtc_ = time;

  time->add_on_time_sync_callback([this]() {
    ESP_LOGD(TAG, "updating Tetris Time from first time sync");
    this->updateTime();
  });
}

void TetrisAnimation::loop() {
  this->updateTime();
}

void TetrisAnimation::reset() {
    this->last_hour_ = 99; // makes the first time invalid
    this->updateTime(true);
}

void TetrisAnimation::set_scale(int scale) {
  this->tetris_.scale = scale;
}

void TetrisAnimation::draw() {
  int x_pos = (this->tetris_.display->get_width() - (this->tetris_.calculateWidth()*this->tetris_.scale)) / 2;
  x_pos /= this->tetris_.scale;

  int y_pos = (this->tetris_.display->get_height() + (TETRIS_Y_DROP_DEFAULT*this->tetris_.scale)) / 2;

  this->tetris_.drawNumbers(x_pos, y_pos, true);
}

void TetrisAnimation::updateTime(bool force_refresh) {
  auto time = this->rtc_->now();
  if (!time.is_valid()) return;
  if (time.minute != this->last_min_ || time.hour != this->last_hour_) {
    sprintf(this->time_buffer_, "%02d:%02d", time.hour, time.minute);
    ESP_LOGD(TAG, "updating Tetris Time to: %s", this->time_buffer_);
    this->tetris_.setTime(this->time_buffer_, force_refresh);
    this->last_hour_ = time.hour;
    this->last_min_ = time.minute;
  }
}

void TetrisAnimation::dump_config() {
  //LOG_DISPLAY("  ", "Display", this->tetris.display);
  //ESP_LOGCONFIG(TAG, "Time Source", this->time_source);
  ESP_LOGCONFIG(TAG, "Scale: %d", this->tetris_.scale);
}

} // namespace tetris_animation
} // namespace esphome
