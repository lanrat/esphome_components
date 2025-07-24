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
    
    // Validate length to prevent buffer overflow
    size_t len = strlen(str);
    if (len != 6) {
        ESP_LOGE(__FUNCTION__, "Invalid hex color length: %zu (expected 6)", len);
        return esphome::Color(0);
    }
    
    // Validate that all characters are valid hex digits
    for (size_t i = 0; i < 6; i++) {
        if (!isxdigit(str[i])) {
            ESP_LOGE(__FUNCTION__, "Invalid hex character '%c' at position %zu", str[i], i);
            return esphome::Color(0);
        }
    }
    
    int matched = sscanf(str, "%02x%02x%02x", &r, &g, &b);
    if (matched != 3) {
        ESP_LOGE(__FUNCTION__, "Unable to parse \"%s\" as hex color", str);
        return esphome::Color(0);
    }
    return esphome::Color(r,g,b);
}

// Get the width (in pixels) of a char string using a given Font
int get_text_width(esphome::display::BaseFont *font, const char *text) {
  int width, x_offset, baseline, height;
  font->measure(text, &width, &x_offset, &baseline, &height);
  return width;
};