#include "game_of_life.h"
#include "esphome/core/log.h"

namespace esphome {
namespace game_of_life {

static const char *const TAG = "game_of_life";

void GameOFLife::setup() {
  this->iteration = 0;
  this->alive = 0;
  this->alive_same_count = 0;
  this->topWeight = 0;

  // TODO unsure if this may need to move to a constructor
  this->set_color_off(color_off);
  this->set_color_age_1(color_age_1);
  this->set_color_age_2(color_age_2);
  this->set_color_age_n(color_age_n);
}

void GameOFLife::set_color_off(esphome::Color & color) {
  this->color_off = color;
}

void GameOFLife::set_color_age_1(esphome::Color & color) {
  this->color_age_1 = color;
}

void GameOFLife::set_color_age_2(esphome::Color & color) {
  this->color_age_2 = color;
}

void GameOFLife::set_color_age_n(esphome::Color & color) {
  this->color_age_n = color;
}


void GameOFLife::set_display(display::Display *) {
  this->display = display;
  this->rows = this->display->get_width();
  this->cols = this->display->get_height();

  // Initializing a single row
	std::vector<char> row(this->cols, AGE_DEAD);
  row.shrink_to_fit();
  // Initializing the 2-D vector
	this->current_state = std::vector<std::vector<char>>(this->rows, row);
  current_state.shrink_to_fit();
}

void GameOFLife::set_starting_density(int starting_density) {
  this->starting_density = starting_density;
}

uint GameOFLife::get_population() {
  return this->alive;
}

uint GameOFLife::get_top_population() {
  return this->topWeight;
}

uint GameOFLife::get_iteration() {
  return this->iteration;
}

void GameOFLife::reset() {
  int num;
  this->mutex.lock();
  for (int r = 0; r < this->rows; r++) {
    for (int c = 0; c < this->cols; c++) {
      num = random(1, 100);
      if (num < starting_density) {
        this->current_state[r][c] = AGE_1;
        this->alive++;
      } else {
        this->current_state[r][c] = AGE_DEAD;
      }
    }
  }
  this->mutex.unlock();

  this->topWeight = 0;
  this->iteration = 0;
  this->alive_same_count = 0;
}

void GameOFLife::render() {
  this->mutex.lock();

  for (int r = 0; r < this->rows; r++) {     // for each row
    for (int c = 0; c < this->cols; c++) {   // and each column
      auto value = this->current_state[r][c];
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
      this->display->draw_pixel_at(c,r,color);

    }
  }
  this->mutex.unlock();
}

void GameOFLife::spark_of_life() {
  int num;
  bool value;

  for (int r = 0; r < this->rows; r++) {
    for (int c = 0; c < this->cols; c++) {
      num = random(1, 100);

      value = num >= 90;
      if (value == 1 && this->current_state[r][c] == AGE_DEAD) {              // only add new points, don't remove any
        this->mutex.lock();
        this->current_state[r][c] = AGE_1;
        this->mutex.unlock();
      }
    }
  }
}

void GameOFLife::nextIteration() {
  int x;
  int y;
  char value;
  //char next[this->rows][this->cols]; // stores the next state of the cells
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

            if (this->current_state[y][x]) {
              liveNeighbors++;
            }
          }
        }
      }

      // apply the rules
      if (this->current_state[r][c] && liveNeighbors >= 2 && liveNeighbors <= 3) { // live cells with 2 or 3 neighbors remain alive
        value = this->current_state[r][c];
        if (value < AGE_n) {
          value++;
        }
        weight++;
      } else if (!this->current_state[r][c] && liveNeighbors == 3) { // dead cells with 3 neighbors become alive
        value = AGE_1;
        weight++;
      } else {
        value = AGE_DEAD;
      }

      next[r][c] = value;
    }
  }

  // discard the old state and keep the new one
  this->mutex.lock();
  // TODO refractor this to use vectors
  //memcpy(this->current_state, next, sizeof next);
  this->current_state.swap(next);
  if (this->alive == weight) {
    this->alive_same_count++;
  } else {
    this->alive_same_count = 0;
    this->alive = weight;
  }
  this->mutex.unlock();

  this->iteration++;

  if (weight >= this->topWeight) this->topWeight=weight;
}

void GameOFLife::loop() {
  // TODO speed control
  // TODO polling componenet?
  // update_interval?

  // do not iterate game state when running spark_of_life, or the component will take too long

  // if the number of alive cells has remained constant for a while, add some random mutations to keep things interesting
  if (alive_same_count > ((this->cols*this->rows) /6)) {
    ESP_LOGD(TAG, "SparkOfLife:  alive_same_count: %d iteration: %d", this->alive_same_count, this->iteration);
    this->alive_same_count = 0;
    if ( this->iteration > 20) this->spark_of_life();  
  } else {
    this->nextIteration();
  }
}

void GameOFLife::dump_config() {
  ESP_LOGCONFIG(TAG, "Starting Density: %d", this->starting_density);
}

} // namespace game_of_life
} // namespace esphome
