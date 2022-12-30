#include "image_cpu.h"

#include <fstream>

#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS true
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma warning(pop)

#include <json/json.hpp>



quavis::ImageCPU::ImageCPU(const glm::ivec2 &dimensions)
  : dimensions_{dimensions}
  , pixels_(dimensions.x * dimensions.y * 4)
  , pixels_ptr_{pixels_.data()}
{
}

quavis::ImageCPU::ImageCPU(const glm::ivec2 &dimensions, std::vector<float> &&pixels)
  : dimensions_(dimensions)
  , pixels_(std::move(pixels))
  , pixels_ptr_{pixels_.data()}
{
  if (dimensions_.x * dimensions_.y * 4 != static_cast<int>(pixels_.size())) {
    throw std::logic_error("pixels vector size must match dimensions");
  }
}

quavis::ImageCPU::ImageCPU(const std::string &file_name)
{
  int comp;
  pixels_ptr_ = stbi_loadf(file_name.c_str(), &dimensions_.x, &dimensions_.y, &comp, 4);

  if (pixels_ptr_ == nullptr) {
    logger_->error("Could not load image {0}", file_name);
    throw std::runtime_error("Could not load image");
  }
}

quavis::ImageCPU::~ImageCPU()
{
  release_pixel_data();
}

void quavis::ImageCPU::write_to_file(const std::string &filename, const StoreFormat format)
{
  logger_->debug("Write image observation {}", filename);

  if (format == IMAGE_CPU_STORE_PNG) {
    // we need to convert
    std::vector<uint8_t> pixelByte;

    const auto p    = get_pixel_data();
    const auto size = pixel_count();

    pixelByte.reserve(size);

    for (size_t i = 0; i < size; i++) {
      pixelByte.push_back(static_cast<uint8_t>(p[i] * 255.0f));
    }

    stbi_write_png(filename.c_str(), dimensions_.x, dimensions_.y, 4, pixelByte.data(), 0);
  } else if (format == IMAGE_CPU_STORE_HDR) {
    stbi_write_hdr(filename.c_str(), dimensions_.x, dimensions_.y, 4, get_pixel_data());
  } else if (format == IMAGE_CPU_STORE_JSON) {
    auto result_ptr = std::make_shared<nlohmann::json>();
    auto &result = *result_ptr;
    // use vector here so we can not save a loaded file
    assert(get_pixel_data() == pixels_.data());
    result = pixels_;
    std::ofstream out_file(filename);
    out_file << *result_ptr;
  } else if (format == IMAGE_CPU_STORE_CBOR) {
    auto result_ptr = std::make_shared<nlohmann::json>();
    auto &result = *result_ptr;
    // use vector here so we can not save a loaded file
    assert(get_pixel_data() == pixels_.data());
    result = pixels_;
    std::ofstream out_file(filename, std::ios::out | std::ios::binary);
    auto data = nlohmann::json::to_cbor(*result_ptr);
    out_file.write(reinterpret_cast<const char*>(data.data()), data.size());
  } else if (format == IMAGE_CPU_STORE_DATA) {
    std::ofstream out_file(filename, std::ios::out | std::ios::binary);
    out_file.write(reinterpret_cast<const char*>(get_pixel_data()), image_byte_size());
  }

  last_filename_ = filename;
  logger_->debug("Write image observation {} done", filename);
}

std::shared_future<std::string> quavis::ImageCPU::wrtie_and_release_async(const std::string &filename, const StoreFormat format)
{
  if (future_last_filename_.valid()) {
    return future_last_filename_;
  }

  auto res = std::async([this, filename, format]() {
    this->write_to_file(filename, format);
    this->release_pixel_data();
    return filename;
  });

  future_last_filename_ = res.share();

  return future_last_filename_;
}

void quavis::ImageCPU::release_pixel_data()
{
  if (pixels_ptr_ != pixels_.data()) {
    stbi_image_free(pixels_ptr_);
  }

  pixels_.resize(0);
  pixels_.shrink_to_fit();
  pixels_ptr_ = pixels_.data();
}
