#include "tetris.h"
#include "esphome/components/time/automation.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "esphome/core/base_automation.h"


namespace esphome {
namespace tetris_animation {

static const char *const TAG = "tetris_animation";


void TetrisAnimation::setup() {
    //this->tetris = TetrisMatrixDraw(this->display);
    this->last_hour = 99; // makes the first time invalid
}

void TetrisAnimation::set_display(display::Display *display) {
  this->display = display;
  this->tetris.display = display;
}

void TetrisAnimation::set_time_source(time::RealTimeClock *time) {
  this->time_source = time;

  time->add_on_time_sync_callback([this]() {
    ESP_LOGD(TAG, "updating Tetris Time from first time sync");
    this->updateTime();
  });

  // TODO apparently its more efficient to just loop and check that seconds == 0.
  
  // create cron to fire every minute.
  auto cron_trigger = new time::CronTrigger(time);
  cron_trigger->add_second(0); // fire at the 0th second


  App.register_component(cron_trigger);
  auto lambda_action = new LambdaAction<>([=]() -> void {
    ESP_LOGD(TAG, "updating Tetris Time from Cron");
    this->updateTime();
  });
  auto automation = new Automation<>(cron_trigger);
  automation->add_actions({lambda_action});
}

void TetrisAnimation::set_scale(int scale) {
  this->scale = scale;
}

void TetrisAnimation::draw() {
    int x_pos = (this->display->get_width() - (this->tetris.calculateWidth()*this->tetris.scale)) / 2;
    x_pos /= this->tetris.scale;

    int y_pos = (this->display->get_height() + (TETRIS_Y_DROP_DEFAULT*this->tetris.scale)) / 2;

    this->tetris.drawNumbers(x_pos, y_pos, true);
}

void TetrisAnimation::updateTime() {
    auto time = this->time_source->now();
        if (time.minute != this->last_min && time.hour != this->last_hour) {
        sprintf(this->time_buffer, "%02d:%02d", time.hour, time.minute);
        this->tetris.scale = this->scale;  // must be called before setText, setTime or setNumbers
        this->tetris.setTime(this->time_buffer);
        this->last_hour = time.hour;
        this->last_min = time.minute;
    }
}

void TetrisAnimation::dump_config() {
  //LOG_DISPLAY("  ", "Display", this->display);
  //ESP_LOGCONFIG(TAG, "Time Source", this->time_source);
  ESP_LOGCONFIG(TAG, "Scale: %d", this->scale);
}

} // namespace tetris_animation
} // namespace esphome
