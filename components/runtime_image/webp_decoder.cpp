#include "webp_decoder.h"
#ifdef USE_RUNTIME_IMAGE_WEBP

#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cinttypes>
#include <cstring>
#include <memory>
#include <webp/demux.h>

#ifdef USE_ESP_IDF
#include "esp_task_wdt.h"
#endif

static const char *const TAG = "image_decoder.webp";

namespace esphome::runtime_image {

// RIFF header: "RIFF" <uint32 little-endian payload size> "WEBP"
static constexpr size_t RIFF_HEADER_SIZE = 12;

static void feed_wdt() {
  // Decoding many frames can take a while; feed the watchdog if this task has one
#ifdef USE_ESP_IDF
  if (esp_task_wdt_status(nullptr) == ESP_OK) {
#endif
    App.feed_wdt();
#ifdef USE_ESP_IDF
  }
#endif
}

/**
 * @brief Size of the whole file according to its RIFF header, or 0 if the header is not there yet
 * or is not a WebP header.
 */
static size_t riff_file_size(const uint8_t *buffer, size_t size) {
  if (size < RIFF_HEADER_SIZE || memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WEBP", 4) != 0) {
    return 0;
  }
  uint32_t payload = buffer[4] | (buffer[5] << 8) | (buffer[6] << 16) | (static_cast<uint32_t>(buffer[7]) << 24);
  // The payload is padded to an even size, but the file length is 8 + payload either way
  return static_cast<size_t>(payload) + 8;
}

int HOT WebpDecoder::decode(uint8_t *buffer, size_t size) {
  // libwebp needs the complete file. Use the expected size if the server sent one,
  // otherwise read the file length from the RIFF header (chunked transfer).
  size_t needed = this->expected_size_;
  if (needed == 0) {
    if (size < RIFF_HEADER_SIZE) {
      return 0;
    }
    needed = riff_file_size(buffer, size);
    if (needed == 0) {
      ESP_LOGE(TAG, "Not a WebP file");
      return DECODE_ERROR_INVALID_TYPE;
    }
  }
  if (size < needed) {
    ESP_LOGV(TAG, "Download not complete. Size: %zu/%zu", size, needed);
    return 0;
  }

  WebPData data;
  WebPDataInit(&data);
  data.bytes = buffer;
  data.size = needed;

  WebPAnimDecoderOptions options;
  if (!WebPAnimDecoderOptionsInit(&options)) {
    ESP_LOGE(TAG, "libwebp version mismatch");
    return DECODE_ERROR_INTERNAL_DECODER_ERROR;
  }
  options.color_mode = MODE_RGBA;
  options.use_threads = 0;

  std::unique_ptr<WebPAnimDecoder, decltype(&WebPAnimDecoderDelete)> decoder(WebPAnimDecoderNew(&data, &options),
                                                                             &WebPAnimDecoderDelete);
  if (!decoder) {
    ESP_LOGE(TAG, "Could not create WebP decoder");
    return DECODE_ERROR_INVALID_TYPE;
  }

  WebPAnimInfo info;
  if (!WebPAnimDecoderGetInfo(decoder.get(), &info)) {
    ESP_LOGE(TAG, "Could not read WebP info");
    return DECODE_ERROR_INVALID_TYPE;
  }
  ESP_LOGD(TAG, "Image size: %" PRIu32 "x%" PRIu32 ", frames: %" PRIu32 ", loops: %" PRIu32, info.canvas_width,
           info.canvas_height, info.frame_count, info.loop_count);
  if (info.frame_count == 0 || info.canvas_width == 0 || info.canvas_height == 0) {
    ESP_LOGE(TAG, "WebP has no frames");
    return DECODE_ERROR_INVALID_TYPE;
  }

  if (!this->set_size(info.canvas_width, info.canvas_height, info.frame_count)) {
    return DECODE_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t frame = 0; frame < info.frame_count; frame++) {
    feed_wdt();
    uint8_t *pixels;
    int timestamp;
    // Each call returns the fully composited canvas for the next frame
    if (!WebPAnimDecoderGetNext(decoder.get(), &pixels, &timestamp) || pixels == nullptr) {
      ESP_LOGE(TAG, "Error decoding frame %" PRIu32 "/%" PRIu32, frame + 1, info.frame_count);
      return DECODE_ERROR_INTERNAL_DECODER_ERROR;
    }
    const uint8_t *p = pixels;
    for (uint32_t y = 0; y < info.canvas_height; y++) {
      for (uint32_t x = 0; x < info.canvas_width; x++, p += 4) {
        this->draw(x, y, 1, 1, Color(p[0], p[1], p[2], p[3]), frame);
      }
    }
  }

  this->decoded_bytes_ = size;
  this->finished_ = true;
  return size;
}

}  // namespace esphome::runtime_image

#endif  // USE_RUNTIME_IMAGE_WEBP
