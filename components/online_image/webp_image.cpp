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
 * @brief Callback method that will be called by the JPEGDEC engine when a chunk
 * of the image is decoded.
 *
 * @param jpeg  The JPEGDRAW object, including the context data.
 */
//static int draw_callback(JPEGDRAW *jpeg) {
static int draw_callback(ImageDecoder *decoder, uint8_t *pix, uint32_t width, uint32_t height) {
  ESP_LOGD(TAG, "draw_callback()");

  static int ixR = 0;
  static int ixG = 1;
  static int ixB = 2;
  static int ixA = 3;
  static int channels = 4;


  for (unsigned int y = 0; y < height; y++) {
    ESP_LOGD(TAG, "IDF-draw-line: %u", y);
    for (unsigned int x = 0; x < width; x++) {
      const uint8_t *p = &pix[(y * width + x) * channels];
      uint8_t r = p[ixR];
      uint8_t g = p[ixG];
      uint8_t b = p[ixB];
      Color color(r, g, b, p[ixA]);
      decoder->draw(x, y, 1, 1, color);
    }
  }

  ESP_LOGD(TAG, "IDF-draw-B");


  // ImageDecoder *decoder = (ImageDecoder *) jpeg->pUser;

  // // Some very big images take too long to decode, so feed the watchdog on each callback
  // // to avoid crashing.
  // App.feed_wdt();
  // size_t position = 0;
  // for (size_t y = 0; y < jpeg->iHeight; y++) {
  //   for (size_t x = 0; x < jpeg->iWidth; x++) {
  //     auto rg = decode_value(jpeg->pPixels[position++]);
  //     auto ba = decode_value(jpeg->pPixels[position++]);
  //     Color color(rg[1], rg[0], ba[1], ba[0]);

  //     if (!decoder) {
  //       ESP_LOGE(TAG, "Decoder pointer is null!");
  //       return 0;
  //     }
  //     decoder->draw(jpeg->x + x, jpeg->y + y, 1, 1, color);
  //   }
  // }
  return 1;
}

int WebpDecoder::prepare(size_t download_size) {
  ESP_LOGD(TAG, "prepare size: %d", download_size);
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

  ESP_LOGD(TAG,"IDF-A");

  this->decoder_ = WebPAnimDecoderNew(&webpData, &decoderOptions);
  if (this->decoder_ == NULL) {
    ESP_LOGE(TAG, "Could not create WebP decoder");
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  ESP_LOGD(TAG,"IDF-B");

  //WebPAnimInfo animation;
  if (!WebPAnimDecoderGetInfo(this->decoder_, &this->animation_)) {
    ESP_LOGE(TAG, "Could not get WebP animation");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return DECODE_ERROR_UNSUPPORTED_FORMAT;
  }

  ESP_LOGD(TAG,"IDF-C");

  ESP_LOGD(TAG, "WebPAnimDecoderGetInfo size: %dx%d, loops: %d, frames: %d, bgcolor: #%X", animation_.canvas_width, animation_.canvas_height, animation_.loop_count, animation_.frame_count, animation_.bgcolor);

  if (!this->set_size( animation_.canvas_width, animation_.canvas_height)) {
    ESP_LOGE(TAG,"could not allocate enough memory");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return DECODE_ERROR_OUT_OF_MEMORY;
  }

  ESP_LOGD(TAG,"IDF-D");

  if (animation_.frame_count == 0) {
    ESP_LOGE(TAG, "Animation has 0 frames");
    WebPAnimDecoderDelete(this->decoder_);
    this->decoder_ = NULL;
    return 0;
  }

  ESP_LOGD(TAG,"IDF-E");

  uint8_t *pix;
  int timestamp;
  WebPAnimDecoderGetNext(this->decoder_, &pix, &timestamp);

  ESP_LOGD(TAG, "IDF-draw-A, ts: %d", timestamp);


  draw_callback(this, pix, this->animation_.canvas_width, this->animation_.canvas_height);

  ESP_LOGD(TAG,"IDF-F");


  // TODO better cleanup when done
  WebPAnimDecoderDelete(this->decoder_);
  this->decoder_ = NULL;

  ESP_LOGD(TAG,"IDF-G");

  this->decoded_bytes_ = size;
  return size;
}

}  // namespace online_image
}  // namespace esphome

#endif  // USE_ONLINE_IMAGE_WEBP_SUPPORT
