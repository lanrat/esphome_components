/*
Copyright (c) 2018 Tobias Blum . All right reserved.

Tetris Matrix Clock

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "TetrisMatrixDraw.h"
#include "TetrisNumbers.h"
#include "TetrisLetters.h"

// this lets esphome logging functions work here
using esphome::esp_log_printf_;

namespace esphome {
namespace tetris_animation {

static const char *const TAG = "TetrisMatrixDraw";

TetrisMatrixDraw::TetrisMatrixDraw()	{
    resetNumStates();
}

void TetrisMatrixDraw::drawChar(std::string letter, uint8_t x, uint8_t y, esphome::Color color, esphome::display::BaseFont *font)
{
    this->display->printf(x, y, font, color, letter.c_str());
}

// *********************************************************************
// Draws a brick shape at a given position
// *********************************************************************
void TetrisMatrixDraw::drawShape(int block_type, esphome::Color color, int x_pos, int y_pos, int num_rot)
{
  // Square
  if (block_type == 0)
  {
    this->display->draw_pixel_at(x_pos, y_pos, color);
    this->display->draw_pixel_at(x_pos + 1, y_pos, color);
    this->display->draw_pixel_at(x_pos, y_pos - 1, color);
    this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
  }

  // L-Shape
  if (block_type == 1)
  {
    if (num_rot == 0)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
    }
    if (num_rot == 1)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos - 1, color);
    }
    if (num_rot == 2)
    {
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 2, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
    }
    if (num_rot == 3)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos - 1, color);
    }
  }

  // L-Shape (reverse)
  if (block_type == 2)
  {
    if (num_rot == 0)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 2, color);
    }
    if (num_rot == 1)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
    }
    if (num_rot == 2)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 2, color);
    }
    if (num_rot == 3)
    {
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
    }
  }

  // I-Shape
  if (block_type == 3)
  {
    if (num_rot == 0 || num_rot == 2)
    { // Horizontal
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
      this->display->draw_pixel_at(x_pos + 3, y_pos, color);
    }
    if (num_rot == 1 || num_rot == 3)
    { // Vertical
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
      this->display->draw_pixel_at(x_pos, y_pos - 3, color);
    }
  }

  // S-Shape
  if (block_type == 4)
  {
    if (num_rot == 0 || num_rot == 2)
    {
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
    }
    if (num_rot == 1 || num_rot == 3)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos - 1, color);
    }
  }

  // S-Shape (reversed)
  if (block_type == 5)
  {
    if (num_rot == 0 || num_rot == 2)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 2, color);
    }
    if (num_rot == 1 || num_rot == 3)
    {
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
    }
  }

  // Half cross
  if (block_type == 6)
  {
    if (num_rot == 0)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
    }
    if (num_rot == 1)
    {
      this->display->draw_pixel_at(x_pos, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos, y_pos - 2, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
    }
    if (num_rot == 2)
    {
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 2, y_pos - 1, color);
    }
    if (num_rot == 3)
    {
      this->display->draw_pixel_at(x_pos + 1, y_pos, color);
      this->display->draw_pixel_at(x_pos, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
      this->display->draw_pixel_at(x_pos + 1, y_pos - 2, color);
    }
  }

   // Corner-Shape 
   if (block_type == 7)
   {
     if (num_rot == 0)
     {
       this->display->draw_pixel_at(x_pos, y_pos, color);
       this->display->draw_pixel_at(x_pos + 1, y_pos, color);
       this->display->draw_pixel_at(x_pos, y_pos - 1, color);
     }
     if (num_rot == 1)
     {
       this->display->draw_pixel_at(x_pos, y_pos, color);
       this->display->draw_pixel_at(x_pos, y_pos - 1, color);
       this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
     }
     if (num_rot == 2)
     {
       this->display->draw_pixel_at(x_pos + 1 , y_pos, color);
       this->display->draw_pixel_at(x_pos + 1 , y_pos - 1, color);
       this->display->draw_pixel_at(x_pos, y_pos - 1, color);
     }
     if (num_rot == 3)
     {
       this->display->draw_pixel_at(x_pos, y_pos, color);
       this->display->draw_pixel_at(x_pos + 1, y_pos , color);
       this->display->draw_pixel_at(x_pos + 1, y_pos - 1, color);
     }
   }
}

void TetrisMatrixDraw::drawLargerBlock(int x_pos, int y_pos, int scale, esphome::Color color){
  this->display->filled_rectangle(x_pos, y_pos, scale, scale, color);
  if(drawOutline){
    this->display->rectangle(x_pos, y_pos, scale, scale, this->outLineColour);
  }
}

void TetrisMatrixDraw::drawLargerShape(int scale, int block_type, esphome::Color color, int x_pos, int y_pos, int num_rot)
{
  int offset1 = 1 * scale;
  int offset2 = 2 * scale;
  int offset3 = 3 * scale;

  // Square
  if (block_type == 0)
  {
    this->drawLargerBlock(x_pos, y_pos, scale, color);
    this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
    this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
    this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
    return;
  }

  // L-Shape
  if (block_type == 1)
  {
    if (num_rot == 0)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      return;
    }
    if (num_rot == 1)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos - offset1, scale, color);
      return;
    }
    if (num_rot == 2)
    {
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset2, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      return;
    }
    if (num_rot == 3)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos - offset1, scale, color);
      return;
    }
  }

  // L-Shape (reverse)
  if (block_type == 2)
  {
    if (num_rot == 0)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset2, scale, color);
      return;
    }
    if (num_rot == 1)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      return;
    }
    if (num_rot == 2)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset2, scale, color);
      return;
    }
    if (num_rot == 3)
    {
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      return;
    }
  }

  // I-Shape
  if (block_type == 3)
  {
    if (num_rot == 0 || num_rot == 2)
    { // Horizontal
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset3, y_pos, scale, color);
      return;
    }
    //if (num_rot == 1 || num_rot == 3)
    //{ // Vertical
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset3, scale, color);
      return;
    //}
  }

  // S-Shape
  if (block_type == 4)
  {
    if (num_rot == 0 || num_rot == 2)
    {
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      return;
    }
    //if (num_rot == 1 || num_rot == 3)
    //{
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos - offset1, scale, color);
      return;
    //}
  }

  // S-Shape (reversed)
  if (block_type == 5)
  {
    if (num_rot == 0 || num_rot == 2)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset2, scale, color);
      return;
    }
    //if (num_rot == 1 || num_rot == 3)
    //{
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      return;
    //}
  }

  // Half cross
  if (block_type == 6)
  {
    if (num_rot == 0)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      return;
    }
    if (num_rot == 1)
    {
      this->drawLargerBlock(x_pos, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset2, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      return;
    }
    if (num_rot == 2)
    {
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset2, y_pos - offset1, scale, color);
      return;
    }
    //if (num_rot == 3)
    //{
      this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
      this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
      this->drawLargerBlock(x_pos + offset1, y_pos - offset2, scale, color);
    //}
  }

   // Corner-Shape 
   if (block_type == 7)
   {
     if (num_rot == 0)
     {
       this->drawLargerBlock(x_pos, y_pos, scale, color);
       this->drawLargerBlock(x_pos + offset1, y_pos, scale, color);
       this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
       return;
     }
     if (num_rot == 1)
     {
       this->drawLargerBlock(x_pos, y_pos, scale, color);
       this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
       this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
       return;
     }
     if (num_rot == 2)
     {
       this->drawLargerBlock(x_pos + offset1 , y_pos, scale, color);
       this->drawLargerBlock(x_pos + offset1 , y_pos - offset1, scale, color);
       this->drawLargerBlock(x_pos, y_pos - offset1, scale, color);
       return;
     }
     //if (num_rot == 3)
     //{
       this->drawLargerBlock(x_pos, y_pos, scale, color);
       this->drawLargerBlock(x_pos + offset1, y_pos , scale, color);
       this->drawLargerBlock(x_pos + offset1, y_pos - offset1, scale, color);
     //}
   }
}

void TetrisMatrixDraw::setNumState(int index, int value, int x_shift)
{
    if(index < TETRIS_MAX_NUMBERS) {
      //ESP_LOGD(TAG, "setNumState: %d", value);
      this->num_states[index].num_to_draw = value;
      this->num_states[index].x_shift = x_shift;
      this->num_states[index].fall_index = 0;
      this->num_states[index].block_index = 0;
    }
}

void TetrisMatrixDraw::setTime(std::string time, bool forceRefresh)
{
    this->sizeOfValue = 4;
    // remove all ':' from string
    time.erase(std::remove(time.begin(), time.end(), ':'), time.end());
    ESP_LOGD(TAG, "setTime str: '%s'", time.c_str());
    for (uint8_t pos = 0; pos < 4; pos++)
    {
      int xOffset = pos * TETRIS_DISTANCE_BETWEEN_DIGITS * this->scale;
      if(pos >= 2){
        xOffset += (3 * this->scale);
      }
      std::string individualNumber = time.substr(pos, 1);
      int number = (individualNumber != " ") ? std::stoi(individualNumber) : -1;
      //ESP_LOGD(TAG, "setTime num: '%s' -> int %d", individualNumber.c_str(), number);
      // Only change the number if its different or being forced
      if (forceRefresh || number != this->num_states[pos].num_to_draw)
      {
        setNumState(pos, number, xOffset);
      }
    }
}

void TetrisMatrixDraw::setNumbers(int value, bool forceRefresh)
{
  std::string strValue = std::to_string(value);
  if(strValue.length() <= TETRIS_MAX_NUMBERS){
    this->sizeOfValue = strValue.length();
    int currentXShift = 0;
    for (uint8_t pos = 0; pos < this->sizeOfValue; pos++)
    {
      currentXShift = TETRIS_DISTANCE_BETWEEN_DIGITS * this->scale * pos;
      int number = std::stoi(strValue.substr(pos, pos + 1));
      // Only change the number if its different or being forced
      if (forceRefresh || number != this->num_states[pos].num_to_draw)
      {
        setNumState(pos, number, currentXShift);
      } else {
        this->num_states[pos].x_shift = currentXShift;
      }
    }
  } else {
    ESP_LOGE(TAG, "Number too long: %d > %d", value, TETRIS_MAX_NUMBERS);
  }
}

void TetrisMatrixDraw::setText(std::string txt, bool forceRefresh)
{
    this->sizeOfValue = txt.length();
    int currentXShift = 0;
    for (uint8_t pos = 0; pos < this->sizeOfValue; pos++)
    {
      currentXShift = TETRIS_DISTANCE_BETWEEN_DIGITS * this->scale * pos;
      char letter = txt.at(pos);
      if (forceRefresh || (int)letter != this->num_states[pos].num_to_draw)
      {
        setNumState(pos, (int)letter, currentXShift);
      } else {
        this->num_states[pos].x_shift = currentXShift;
      }
    }
}

bool TetrisMatrixDraw::drawText(int x, int yFinish)
{
  // For each number position
  bool finishedAnimating = true;

  int scaledYOffset = (this->scale > 1) ? this->scale : 1;
  int y = yFinish - (TETRIS_Y_DROP_DEFAULT * this->scale);

  // For each number position
  for (int numpos = 0; numpos < this->sizeOfValue; numpos++)
  {

    if(num_states[numpos].num_to_draw >= 33)
    {
      // Draw falling shape
      //if (numstates[numpos].blockindex < blocksPerNumber[numstates[numpos].num_to_draw])
      if (num_states[numpos].block_index < blocksPerChar[num_states[numpos].num_to_draw-33])
      {
        finishedAnimating = false;
        fall_instr_let current_fall = getFallingStrByAscii(num_states[numpos].num_to_draw, num_states[numpos].block_index);

        // Handle variations of rotations
        uint8_t rotations = current_fall.num_rot;
        if (rotations == 1)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 2))
          {
            rotations = 0;
          }
        }
        if (rotations == 2)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 3))
          {
            rotations = 0;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 3 * 2))
          {
            rotations = 1;
          }
        }
        if (rotations == 3)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4))
          {
            rotations = 0;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4 * 2))
          {
            rotations = 1;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4 * 3))
          {
            rotations = 2;
          }
        }
        if(this->scale <= 1){
          drawShape(current_fall.block_type, 
                    this->tetrisColors[current_fall.color],
                    x + current_fall.x_pos + num_states[numpos].x_shift, 
                    y + num_states[numpos].fall_index - scaledYOffset, 
                    rotations);
        } else {
          drawLargerShape(this->scale, 
                          current_fall.block_type, 
                          this->tetrisColors[current_fall.color], 
                          x + (current_fall.x_pos * this->scale) + num_states[numpos].x_shift, 
                          y + (num_states[numpos].fall_index * scaledYOffset) - scaledYOffset, 
                          rotations);
        }
        //drawShape(current_fall.blocktype, this->tetrisColors[current_fall.color], x + current_fall.x_pos + numstates[numpos].x_shift, y + numstates[numpos].fallindex - 1, rotations);
        num_states[numpos].fall_index++;

        if (num_states[numpos].fall_index > current_fall.y_stop)
        {
          num_states[numpos].fall_index = 0;
          num_states[numpos].block_index++;
        }
      }

      // Draw already dropped shapes
      if (num_states[numpos].block_index > 0)
      {
        for (int i = 0; i < num_states[numpos].block_index; i++)
        {
          fall_instr_let fallen_block = getFallingStrByAscii(num_states[numpos].num_to_draw, i);
          if(this->scale <= 1){
            drawShape(fallen_block.block_type, 
                      this->tetrisColors[fallen_block.color], 
                      x + fallen_block.x_pos + num_states[numpos].x_shift, 
                      y + fallen_block.y_stop - 1, 
                      fallen_block.num_rot);
          } else {
            drawLargerShape(this->scale, 
                            fallen_block.block_type, 
                            this->tetrisColors[fallen_block.color], 
                            x + (fallen_block.x_pos * this->scale) + num_states[numpos].x_shift, 
                            y + (fallen_block.y_stop * scaledYOffset) - scaledYOffset, 
                            fallen_block.num_rot);
          }
          //drawShape(fallen_block.blocktype, this->tetrisColors[fallen_block.color], x + fallen_block.x_pos + numstates[numpos].x_shift, y + fallen_block.y_stop - 1, fallen_block.num_rot);
        }
      }
    }
    
  }
  return finishedAnimating;
}

bool TetrisMatrixDraw::drawNumbers(int x, int yFinish, bool displayColon)
{
  // For each number position
  bool finishedAnimating = true;

  int scaledYOffset = (this->scale > 1) ? this->scale : 1;
  int y = yFinish - (TETRIS_Y_DROP_DEFAULT * this->scale);

  for (int numpos = 0; numpos < this->sizeOfValue; numpos++)
  {
    if(num_states[numpos].num_to_draw >= 0) 
    {
      // Draw falling shape
      if (num_states[numpos].block_index < blocksPerNumber[num_states[numpos].num_to_draw])
      {
        finishedAnimating = false;
        fall_instr current_fall = getFallingStrByNum(num_states[numpos].num_to_draw, num_states[numpos].block_index);

        // Handle variations of rotations
        uint8_t rotations = current_fall.num_rot;
        if (rotations == 1)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 2))
          {
            rotations = 0;
          }
        }
        if (rotations == 2)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 3))
          {
            rotations = 0;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 3 * 2))
          {
            rotations = 1;
          }
        }
        if (rotations == 3)
        {
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4))
          {
            rotations = 0;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4 * 2))
          {
            rotations = 1;
          }
          if (num_states[numpos].fall_index < (int)(current_fall.y_stop / 4 * 3))
          {
            rotations = 2;
          }
        }

        if(this->scale <= 1){
          drawShape(current_fall.block_type, 
                    this->tetrisColors[current_fall.color],
                    x + current_fall.x_pos + num_states[numpos].x_shift, 
                    y + num_states[numpos].fall_index - scaledYOffset, 
                    rotations);
        } else {
          drawLargerShape(this->scale, 
                          current_fall.block_type, 
                          this->tetrisColors[current_fall.color], 
                          x + (current_fall.x_pos * this->scale) + num_states[numpos].x_shift, 
                          y + (num_states[numpos].fall_index * scaledYOffset) - scaledYOffset, 
                          rotations);
        }
        num_states[numpos].fall_index++;

        if (num_states[numpos].fall_index > current_fall.y_stop)
        {
          num_states[numpos].fall_index = 0;
          num_states[numpos].block_index++;
        }
      }

      // Draw already dropped shapes
      if (num_states[numpos].block_index > 0)
      {
        for (int i = 0; i < num_states[numpos].block_index; i++)
        {
          fall_instr fallen_block = getFallingStrByNum(num_states[numpos].num_to_draw, i);
          if(this->scale <= 1){
            drawShape(fallen_block.block_type, 
                      this->tetrisColors[fallen_block.color], 
                      x + fallen_block.x_pos + num_states[numpos].x_shift, 
                      y + fallen_block.y_stop - 1, 
                      fallen_block.num_rot);
          } else {
            drawLargerShape(this->scale, 
                            fallen_block.block_type, 
                            this->tetrisColors[fallen_block.color], 
                            x + (fallen_block.x_pos * this->scale) + num_states[numpos].x_shift, 
                            y + (fallen_block.y_stop * scaledYOffset) - scaledYOffset, 
                            fallen_block.num_rot);
          }
        }
      }
    }
  }

  if (displayColon)
  {
    this->drawColon(x, y, tetrisWHITE);
  }
  return finishedAnimating;
}

void TetrisMatrixDraw::drawColon(int x, int y, esphome::Color colonColour){
  int colonSize = 2 * this->scale;
  int xColonPos = x + (TETRIS_DISTANCE_BETWEEN_DIGITS * 2 * this->scale);  
  display->filled_rectangle(xColonPos, y + (12 * this->scale), colonSize, colonSize, colonColour);
  display->filled_rectangle(xColonPos, y + (8 * this->scale), colonSize, colonSize, colonColour);
}

int TetrisMatrixDraw::calculateWidth(){
  return (this->sizeOfValue * TETRIS_DISTANCE_BETWEEN_DIGITS) - 1;
}

void TetrisMatrixDraw::resetNumStates(){
    for(int i = 0; i < TETRIS_MAX_NUMBERS; i++){
        this->num_states[i].num_to_draw = -1;
        this->num_states[i].fall_index = 0;
        this->num_states[i].block_index = 0;
        this->num_states[i].x_shift = 0;
    }
}

} // namespace tetris_animation
} // namespace esphome
