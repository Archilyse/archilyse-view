#ifndef QUAVIS_UTILS_IMAGE_CPU
#define QUAVIS_UTILS_IMAGE_CPU

#include <future>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "../logger.h"

namespace quavis {
/// ImageCPU a float 4 channel image on the CPU. Supports read and multithreaded write.
class ImageCPU : UseLogger {
 public:
  /// Creates an 0 initialized image with given dimensions. dimensions.x == width, dimensions.y == height
  ImageCPU(const glm::ivec2 &dimensions);
  /// Creates an image by moving in pixels from the given vector. pixels.size() must be 4*dimensions.x*dimensions.y
  ImageCPU(const glm::ivec2 &dimensions, std::vector<float> &&pixels);
  /// Creates image by loading it from a file
  ImageCPU(const std::string &file_name);

  // not copyable only movable
  ImageCPU(ImageCPU &&other) = default;
  ImageCPU &operator=(ImageCPU &&mE) = default;
  ImageCPU(const ImageCPU &)         = delete;
  ImageCPU &operator=(const ImageCPU &) = delete;

  virtual ~ImageCPU();

  enum StoreFormat { 
    IMAGE_CPU_STORE_PNG = 0,
    IMAGE_CPU_STORE_HDR = 1,
    IMAGE_CPU_STORE_JSON = 2,
    IMAGE_CPU_STORE_CBOR = 3,
    IMAGE_CPU_STORE_DATA = 4
  };
  /// Stores the image to given format and may converts channel data beforehand. Blocking call
  void write_to_file(const std::string &filename, const StoreFormat format = IMAGE_CPU_STORE_PNG);

  /// Stores the image to given format and may converts channel data beforehand, releases image memory afterwards. Non-Blocking with works with a new
  /// thread.
  std::shared_future<std::string> wrtie_and_release_async(const std::string &filename, const StoreFormat format = IMAGE_CPU_STORE_PNG);

  /// returns width (x) and height (y) of the image
  glm::ivec2 get_dimensions() const { return dimensions_; }
  /// number of floats in the image, means 4 * width * height
  size_t pixel_count() const { return dimensions_.x * dimensions_.y * 4; }
  /// size of pixels in bytes
  size_t image_byte_size() const { return pixel_count() * sizeof(pixels_[0]); }

  /// false if pixel data has been released, (e.g after wrtie_and_release_async)
  bool has_pixel_data() { return pixels_.size() > 0 || pixels_.data() != pixels_ptr_; }

  /// the continuous pixel data
  float *get_pixel_data() { return pixels_ptr_; }
  /// the continuous pixel data
  const float *get_pixel_data() const { return pixels_ptr_; }

  /// releases the pixel data
  void release_pixel_data();

  /// returns the filename used in the last save operation (write_and_release_async, write_to_file ) or an empty string
  std::string get_last_filename() const { return last_filename_; }

  /// returns a future when the last save operation was async. NULL otherwise
  std::shared_future<std::string> get_last_filename_future() const { return future_last_filename_; }

 private:
  glm::ivec2 dimensions_;
  std::vector<float> pixels_;
  // stupid read library does not support to let as give our nice for sure leak
  // free vector
  float *pixels_ptr_;

  std::string last_filename_;
  std::shared_future<std::string> future_last_filename_;
};
}  // namespace quavis

#endif
