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
#pragma once


#include "esphome/components/display/display.h"
#include "esphome/core/color.h"

#define TETRIS_MAX_NUMBERS 9

#ifndef TETRIS_DISTANCE_BETWEEN_DIGITS
#define TETRIS_DISTANCE_BETWEEN_DIGITS 7
#endif

#ifndef TETRIS_Y_DROP_DEFAULT
#define TETRIS_Y_DROP_DEFAULT 16
#endif

namespace esphome {
namespace tetris_animation {

// Type that describes the current state of a drawn number
typedef struct
{
  int num_to_draw; // Number to draw (0-9)
  int blockindex;  // The index of the brick (as defined in the falling instructions) that is currently falling
  int fallindex;   // y-position of the brick it already has (incrementing with each step)
  int x_shift;     // x-position of the number relative to the matrix where the number should be placed.
} numstate;

// default colors
static const esphome::Color tetrisRED = esphome::Color(255, 0, 0);
static const esphome::Color tetrisGREEN = esphome::Color(0, 255, 0);
static const esphome::Color tetrisBLUE = esphome::Color(49, 73, 255);
static const esphome::Color tetrisWHITE = esphome::Color(255, 255, 255);
static const esphome::Color tetrisYELLOW = esphome::Color(255, 255, 0);
static const esphome::Color tetrisCYAN = esphome::Color(0, 255, 255);
static const esphome::Color tetrisMAGENTA = esphome::Color(255, 0, 255);
static const esphome::Color tetrisORANGE = esphome::Color(255, 97, 0);
static const esphome::Color tetrisBLACK = esphome::Color(0, 0, 0);

class TetrisMatrixDraw
{
    public:
        TetrisMatrixDraw (esphome::display::Display *display);
        bool drawNumbers(int x = 0, int y = 0, bool displayColon = false);
        bool drawText(int x = 0, int y = 0);
        void drawChar(std::string letter, uint8_t x, uint8_t y, esphome::Color color, esphome::display::BaseFont *font);
        void drawShape(int blocktype, esphome::Color color, int x_pos, int y_pos, int num_rot);
        void drawLargerShape(int scale, int blocktype, esphome::Color color, int x_pos, int y_pos, int num_rot);
        void setTime(std::string time, bool forceRefresh = false);
        void setNumbers(int value, bool forceRefresh = false);
        void setText(std::string txt, bool forceRefresh = false);
        void setNumState(int index, int value, int x_shift);
        void drawColon(int x, int y, esphome::Color colonColour);
        int calculateWidth();
        bool _debug = false;
        int scale = 1;
        bool drawOutline = false;
        esphome::Color outLineColour = tetrisBLACK;

        esphome::Color tetrisColors[9] = {tetrisRED, tetrisGREEN, tetrisBLUE, tetrisWHITE, tetrisYELLOW, tetrisCYAN, tetrisMAGENTA, tetrisORANGE, tetrisBLACK};

    private:
        esphome::display::Display  *display;
        void intialiseColors();
        void resetNumStates();
        void drawLargerBlock(int x_pos, int y_pos, int scale, esphome::Color color);
        numstate numstates[TETRIS_MAX_NUMBERS];
        int sizeOfValue;
};

} // namespace tetris_animation
} // namespace esphome
