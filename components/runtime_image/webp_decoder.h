#pragma once

#include "image_decoder.h"
#include "runtime_image.h"
#include "esphome/core/defines.h"
#ifdef USE_RUNTIME_IMAGE_WEBP

namespace esphome::runtime_image {

/**
 * @brief Image decoder specialization for WebP images, static or animated.
 *
 * libwebp's animation decoder needs the whole file in memory, so this waits
 * until the complete image has been buffered, then decodes every frame into
 * the RuntimeImage's frame buffer.
 */
class WebpDecoder : public ImageDecoder {
 public:
  /**
   * @brief Construct a new WebP Decoder object.
   *
   * @param image The RuntimeImage to decode the stream into.
   */
  WebpDecoder(RuntimeImage *image) : ImageDecoder(image, WEBP) {}
  ~WebpDecoder() override {}

  int HOT decode(uint8_t *buffer, size_t size) override;

  void reset() override {
    ImageDecoder::reset();
    this->finished_ = false;
  }

  /// Decoding is all or nothing, so the size may be unknown (chunked transfer) and still be finished.
  bool is_finished() const override { return this->finished_; }

 protected:
  bool finished_{false};
};

}  // namespace esphome::runtime_image

#endif  // USE_RUNTIME_IMAGE_WEBP
