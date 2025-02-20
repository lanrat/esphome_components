#include "webp_image.h"

#ifdef USE_ONLINE_IMAGE_WEBP_SUPPORT

#include "esphome/components/display/display_buffer.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "online_image.h"
static const char *const TAG = "online_image.webp";

namespace esphome {
namespace online_image {

/**
 * @brief method that will be called to draw each frame to the image buffer.
 *
 * @param decoder The ImageDecoder to draw to
 * @param pix Buffer of pixels for the given frame
 * @param width Image width
 * @param height Image Height
 * @param frame The frame to draw the image to
 */
void draw_frame(ImageDecoder *decoder, uint8_t *pix, uint32_t width, uint32_t height, int frame) {
  static int ixR = 0;
  static int ixG = 1;
  static int ixB = 2;
  static int ixA = 3;
  static int channels = 4;

  for (unsigned int y = 0; y < height; y++) {
    for (unsigned int x = 0; x < width; x++) {
      const uint8_t *p = &pix[(y * width + x) * channels];
      uint8_t r = p[ixR];
      uint8_t g = p[ixG];
      uint8_t b = p[ixB];
      Color color(r, g, b, p[ixA]);
      decoder->draw(x, y, 1, 1, color, frame);
    }
  }
}

int WebpDecoder::prepare(size_t download_size) {
  ImageDecoder::prepare(download_size);
  auto size = this->image_->resize_download_buffer(download_size);
  if (size < download_size) {
    ESP_LOGE(TAG, "Download buffer resize failed!");
    return DECODE_ERROR_OUT_OF_MEMORY;
  }
  return 0;
}

int HOT WebpDecoder::decode(uint8_t *buffer, size_t size) {
  ESP_LOGD(TAG, "decode size: %d", size);
  if (size < this->download_size_) {
    ESP_LOGV(TAG, "Download not complete. Size: %d/%d", size, this->download_size_);
    return 0;
  }

  // Set up WebP decoder
  WebPData webpData;
  WebPDataInit(&webpData);
  webpData.bytes = buffer;
  webpData.size = size;

  WebPAnimDecoderOptions decoderOptions;
  WebPAnimDecoderOptionsInit(&decoderOptions);
  decoderOptions.color_mode = MODE_RGBA;

  this->decoder_ = WebPAnimDecoderNew(&webpData, &decoderOptions);
  if (this->decoder_ == NULL) {
    ESP_LOGE(TAG, "Could not create WebP decoder");
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  if (!WebPAnimDecoderGetInfo(this->decoder_, &this->animation_)) {
    ESP_LOGE(TAG, "Could not get WebP animation");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  ESP_LOGD(TAG, "WebPAnimDecode size: (%dx%d), loops: %d, frames: %d, bgcolor: #%X", animation_.canvas_width, animation_.canvas_height, animation_.loop_count, animation_.frame_count, animation_.bgcolor);

  if (!this->set_size(animation_.canvas_width, animation_.canvas_height, animation_.frame_count)) {
    ESP_LOGE(TAG,"could not allocate enough memory");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return DECODE_ERROR_OUT_OF_MEMORY;
  }

  if (animation_.frame_count == 0) {
    ESP_LOGE(TAG, "Animation has 0 frames");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return 0;
  }

  // iterate over all frames
  for (uint frame = 0; frame < animation_.frame_count; frame++) {
    uint8_t *pix;
    int timestamp;
    if (!WebPAnimDecoderGetNext(this->decoder_, &pix, &timestamp)) {
      ESP_LOGE(TAG,"error parsing webp frame %u/%u", frame, animation_.frame_count);
      WebPAnimDecoderDelete(this->decoder_);
      this->decoder_ = NULL;
      return DECODE_ERROR_UNSUPPORTED_FORMAT;
    }

    draw_frame(this, pix, this->animation_.canvas_width, this->animation_.canvas_height, frame);
  }

  WebPAnimDecoderDelete(this->decoder_);
  this->decoder_ = NULL;

  this->decoded_bytes_ = size;
  return size;
}

}  // namespace online_image
}  // namespace esphome

#endif  // USE_ONLINE_IMAGE_WEBP_SUPPORT
