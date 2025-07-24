#pragma once
#include "esphome/components/display/display.h"

// https://stackoverflow.com/questions/485800/algorithm-for-drawing-an-anti-aliased-circle/485850#485850
// https://stackoverflow.com/questions/64816341/how-do-you-draw-an-antialiased-circular-line-of-a-certain-thickness-how-to-set

// modeled after
// https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.cpp#L4246

#define aa_random(minimum_number, max_number) (rand() % ((max_number) - (minimum_number) + 1) + (minimum_number))

/***************************************************************************************
** Function name:           sqrt_fraction (private function)
** Description:             Smooth graphics support function for alpha derivation
***************************************************************************************/
// Compute the fixed point square root of an integer and
// return the 8 MS bits of fractional part.
// Quicker than sqrt() for processors that do not have an FPU (e.g. RP2040)
inline uint8_t sqrt_fraction(uint32_t num) {
  if (num > (0x40000000)) return 0;
  uint32_t bsh = 0x00004000;
  uint32_t fpr = 0;
  uint32_t osh = 0;

  // Auto adjust from U8:8 up to U15:16
  while (num>bsh) {bsh <<= 2; osh++;}

  do {
    uint32_t bod = bsh + fpr;
    if(num >= bod)
    {
      num -= bod;
      fpr = bsh + bod;
    }
    num <<= 1;
  } while(bsh >>= 1);

  return fpr>>osh;
}

/***************************************************************************************
** Function name:           alphaBlend
** Description:             Blend 24bit foreground and background with optional dither
***************************************************************************************/
esphome::Color alphaBlend(uint8_t alpha, esphome::Color fgc, esphome::Color bgc, uint8_t dither) {
  if (dither) {
    int16_t alphaDither = (int16_t)alpha - dither + aa_random(0, 2*dither+1); // +/-dither randomized
    alpha = (uint8_t)alphaDither;
    if (alphaDither <  0) alpha = 0;
    if (alphaDither >255) alpha = 255;
  }

  bgc.red += ((fgc.red) - bgc.red) * alpha >> 8;
  bgc.green += ((fgc.green) - bgc.green) * alpha >> 8;
  bgc.blue += ((fgc.blue) - bgc.blue) * alpha >> 8;

  return bgc;
}

// fillSmoothCircle
void smooth_filled_circle(esphome::display::Display& it, int32_t x, int32_t y, int32_t r, esphome::Color color, esphome::Color bg_color)
{
  if (r <= 0) return;

  it.horizontal_line(x - r, y, 2 * r + 1, color);
  int32_t xs = 1;
  int32_t cx = 0;

  int32_t r1 = r * r;
  r++;
  int32_t r2 = r * r;
  
  for (int32_t cy = r - 1; cy > 0; cy--) {
    int32_t dy2 = (r - cy) * (r - cy);
    for (cx = xs; cx < r; cx++) {
        int32_t hyp2 = (r - cx) * (r - cx) + dy2;
        if (hyp2 <= r1) break;
        if (hyp2 >= r2) continue;

        uint8_t alpha = ~sqrt_fraction(hyp2);
        if (alpha > 246) break;
        xs = cx;
        if (alpha < 9) continue;

        auto blended_color = alphaBlend(alpha, color, bg_color, 0);
        it.draw_pixel_at(x + cx - r, y + cy - r, blended_color);
        it.draw_pixel_at(x - cx + r, y + cy - r, blended_color);
        it.draw_pixel_at(x - cx + r, y - cy + r, blended_color);
        it.draw_pixel_at(x + cx - r, y - cy + r, blended_color);
    }
    it.horizontal_line(x + cx - r, y + cy - r, 2 * (r - cx) + 1, color);
    it.horizontal_line(x + cx - r, y - cy + r, 2 * (r - cx) + 1, color);
  }
}
