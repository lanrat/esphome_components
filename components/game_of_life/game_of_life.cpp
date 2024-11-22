#include "game_of_life.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace game_of_life {

static const char *const TAG = "game_of_life";

GameOfLife::GameOfLife() {
  // set defaults
  this->set_color_off(color_off);
  this->set_color_age_1(color_age_1);
  this->set_color_age_2(color_age_2);
  this->set_color_age_n(color_age_n);
}

void GameOfLife::setup() {
  this->reset();
}

void GameOfLife::reset() {
  this->alive_ = 0;
  int num;
  this->mutex_.lock();
  for (int r = 0; r < this->rows; r++) {
    for (int c = 0; c < this->cols; c++) {
      num = random(1, 100);
      if (num < this->starting_density_) {
        this->current_state_[r][c] = AGE_1;
        this->alive_++;
      } else {
        this->current_state_[r][c] = AGE_DEAD;
      }
    }
  }
  this->mutex_.unlock();

  this->topWeight_ = 0;
  this->iteration_ = 0;
  this->alive_same_count_ = 0;
}

void GameOfLife::set_color_off(esphome::Color color) {
  this->color_off = color;
}

void GameOfLife::set_color_age_1(esphome::Color color) {
  this->color_age_1 = color;
}

void GameOfLife::set_color_age_2(esphome::Color color) {
  this->color_age_2 = color;
}

void GameOfLife::set_color_age_n(esphome::Color color) {
  this->color_age_n = color;
}

void GameOfLife::set_speed(uint8_t speed) {
  this->speed_ = speed;
  this->set_next_call_ns_();
}

void GameOfLife::set_spark(bool spark) {
  this->run_spark_ = spark;
}

void GameOfLife::set_display(display::Display *display) {
  this->display_ = display;
  this->rows = this->display_->get_height();
  this->cols = this->display_->get_width();

  // Initializing a single row
	std::vector<char> row(this->cols, AGE_DEAD);
  row.shrink_to_fit();
  // Initializing the 2-D vector
	this->current_state_ = std::vector<std::vector<char>>(this->rows, row);
  this->current_state_.shrink_to_fit();
}

void GameOfLife::set_starting_density(int starting_density) {
  this->starting_density_ = starting_density;
}

uint GameOfLife::get_population() {
  return this->alive_;
}

uint GameOfLife::get_top_population() {
  return this->topWeight_;
}

uint GameOfLife::get_iteration() {
  return this->iteration_;
}

uint8_t GameOfLife::get_speed() {
  return this->speed_;
}

void GameOfLife::render() {
  this->mutex_.lock();

  for (int r = 0; r < this->rows; r++) {     // for each row
    for (int c = 0; c < this->cols; c++) {   // and each column
      auto value = this->current_state_[r][c];
      auto color = color_off;
      switch(value) {
        case AGE_1:
          color = color_age_1;
          break;
        case AGE_2:
          color = color_age_2;
          break;
        case AGE_n:
          color = color_age_n;
          break;
      }
      this->display_->draw_pixel_at(c,r,color);

    }
  }
  this->mutex_.unlock();
}

void GameOfLife::spark_of_life() {
  int num;
  bool value;

  for (int r = 0; r < this->rows; r++) {
    for (int c = 0; c < this->cols; c++) {
      num = random(1, 100);

      value = num >= 90;
      if (value == 1 && this->current_state_[r][c] == AGE_DEAD) {              // only add new points, don't remove any
        this->mutex_.lock();
        this->current_state_[r][c] = AGE_1;
        this->mutex_.unlock();
      }
    }
  }
}

void GameOfLife::nextIteration() {
  int x;
  int y;
  char value;
  std::vector<char> row(this->cols, AGE_DEAD);
  row.shrink_to_fit();
	auto next = std::vector<std::vector<char>>(this->rows, row);
  next.shrink_to_fit();
  uint weight = 0;   //total number of on-points

  for (int r = 0; r < this->rows; r++) {     // for each row
    for (int c = 0; c < this->cols; c++) {   // and each column
      // count how many live neighbors this cell has
      int liveNeighbors = 0;
      for (int i = -1; i < 2; i++) {
        y = r + i;
        if (y == -1) {
          y = (this->rows-1);
        } else if (y == this->rows) {
          y = 0;
        }
        for (int j = -1; j < 2; j++) {
          if (i != 0 || j != 0) {
            x = c + j;
            if (x == -1) {
              x = (this->cols-1);
            } else if (x == this->cols) {
              x = 0;
            }

            if (this->current_state_[y][x]) {
              liveNeighbors++;
            }
          }
        }
      }

      // apply the rules
      if (this->current_state_[r][c] && liveNeighbors >= 2 && liveNeighbors <= 3) { // live cells with 2 or 3 neighbors remain alive
        value = this->current_state_[r][c];
        if (value < AGE_n) {
          value++;
        }
        weight++;
      } else if (!this->current_state_[r][c] && liveNeighbors == 3) { // dead cells with 3 neighbors become alive
        value = AGE_1;
        weight++;
      } else {
        value = AGE_DEAD;
      }

      next[r][c] = value;
    }
  }

  // discard the old state and keep the new one
  this->mutex_.lock();
  this->current_state_.swap(next);
  if (this->alive_ == weight) {
    this->alive_same_count_++;
  } else {
    this->alive_same_count_ = 0;
    this->alive_ = weight;
  }
  this->mutex_.unlock();

  this->iteration_++;

  if (weight >= this->topWeight_) this->topWeight_=weight;
}

void GameOfLife::loop() {
  if (!this->running_) return;

  // do not iterate game state when running spark_of_life, or the component will take too long
  // if the number of alive cells has remained constant for a while, add some random mutations to keep things interesting
  if (this->run_spark_ && alive_same_count_ > ((this->cols*this->rows) /6)) {
    ESP_LOGD(TAG, "SparkOfLife:  alive_same_count_: %d iteration: %d", this->alive_same_count_, this->iteration_);
    this->alive_same_count_ = 0;
    if ( this->iteration_ > 20) this->spark_of_life();  
  } else {

    // check if this should run based on speed
    int64_t curr_time_ns = this->get_time_ns_();
    if (curr_time_ns < this->next_call_ns_) {
      return;
    }
    
    // set next time to run
    this->set_next_call_ns_();
    this->nextIteration();
  }
}

void GameOfLife::set_next_call_ns_() {
  auto wait_ms = 1000-((this->speed_)*100);
  ESP_LOGD(TAG, "Waiting for %d ms for next iteration", wait_ms);
  this->next_call_ns_ = (wait_ms * INT64_C(1000000)) + this->get_time_ns_();
}

void GameOfLife::dump_config() {
  ESP_LOGCONFIG(TAG, "Starting Density: %d", this->starting_density_);
  ESP_LOGCONFIG(TAG, "Spark Enable: %d", this->run_spark_);
  ESP_LOGCONFIG(TAG, "Starting Speed: %d", this->speed_);
}

int64_t GameOfLife::get_time_ns_() {
  int64_t time_ms = millis();
  if (this->last_time_ms_ > time_ms) {
    this->millis_overflow_counter_++;
  }
  this->last_time_ms_ = time_ms;

  return (time_ms + ((int64_t) this->millis_overflow_counter_ << 32)) * INT64_C(1000000);
}

} // namespace game_of_life
} // namespace esphome
