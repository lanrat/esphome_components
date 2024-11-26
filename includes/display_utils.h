#pragma once
#include "esphome/components/display/display.h"
#include "esphome/core/color.h"
#include "esphome/core/log.h"

esphome::Color hexToColor(const char *str) {
    if (str == nullptr) {
        ESP_LOGE(__FUNCTION__, "passed null string");
        return esphome::Color(0);
    }
    uint8_t r, g, b;
    if (str[0] == '#') {
        // skip over leading hash if provided
        str++;
    }
    int matched = sscanf(str, "%02x%02x%02x", &r, &g, &b);
    if (matched != 3) {
        ESP_LOGE(__FUNCTION__, "Unable to parse \"%s\" as hex color", str);
    }
    return esphome::Color(r,g,b);
}

// Get the width (in pixels) of a char string using a given Font
int get_text_width(esphome::display::BaseFont *font, const char *text) {
  int width, x_offset, baseline, height;
  font->measure(text, &width, &x_offset, &baseline, &height);
  return width;
};