#pragma once

#include "image_decoder.h"
#include "esphome/core/defines.h"
#ifdef USE_ONLINE_IMAGE_WEBP_SUPPORT
#include <webp/demux.h>

namespace esphome {
namespace online_image {

/**
 * @brief Image decoder specialization for WEBP images.
 */
class WebpDecoder : public ImageDecoder {
 public:
  /**
   * @brief Construct a new JPEG Decoder object.
   *
   * @param display The image to decode the stream into.
   */
  WebpDecoder(OnlineImage *image) : ImageDecoder(image) {}
  ~WebpDecoder() override {}

  int prepare(size_t download_size) override;
  int HOT decode(uint8_t *buffer, size_t size) override;

 protected:
  WebPAnimInfo animation_;
  WebPAnimDecoder *decoder_;
  //JPEGDEC jpeg_{};
  //std::unique_ptr<ImageSource> data_;
};

}  // namespace online_image
}  // namespace esphome

#endif  // USE_ONLINE_IMAGE_WEBP_SUPPORT
