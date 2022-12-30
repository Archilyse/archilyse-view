#ifndef QUAVIS_RENDER_CUBE_IMAGES
#define QUAVIS_RENDER_CUBE_IMAGES

#include <glm/glm.hpp>

#include "../utils/image_cpu.h"
#include "./anvil.h"

namespace quavis {

class CubeImages {
 public:
  /// different ways to use this image cube
  enum Usages {
    USAGE_COLOR_ATTACHMENT = 0,
    USAGE_DEPTH_ATTACHMENT = 1,
    USAGE_COLOR_TEXTURE    = 2,

  };

  /// create an empty texture that can be used for usage
  CubeImages(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2 &render_size_, const Usages usage);

  /// creates a cube image for pre initialized with ImageCPU faces, can be used as color texture.
  CubeImages(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const std::vector<std::shared_ptr<ImageCPU>> &faces);

  /// upload faces from cpu to gpu
  void upload_images(const std::vector<std::shared_ptr<ImageCPU>> &faces);

  /// prepares the image cube to be used as an attachment next.
  void prepare_for_render(std::shared_ptr<Anvil::PrimaryCommandBuffer> &command_buffer, std::shared_ptr<Anvil::Queue> &queue);

  /// get cube map view
  std::shared_ptr<Anvil::ImageView> get_view_cubemap();
  /// get a 2d view of one face
  std::shared_ptr<Anvil::ImageView> get_view_single_face(uint32_t face);
  /// get a texture array view with 6 layers
  std::shared_ptr<Anvil::ImageView> get_view_texture_array(uint32_t baseLayer = 0, uint32_t layers = 6);
  /// get a storage image view with 6 layers
  std::shared_ptr<Anvil::ImageView> get_storage_image_view(std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                                           std::shared_ptr<Anvil::Queue> queue);

  /// clear image, normally not needed use render pass clear instead.
  void clear_images(const glm::vec4 &color);

  /// upload the image as one single cpu image, pretty false gives two row of each 3 images for all 6 faces, pretty true gives an unfolded view.
  std::shared_ptr<ImageCPU> retrieve_images(bool pretty = false);

  const VkFormat get_image_format() const;
  const VkImageUsageFlags get_image_usage() const;
  const VkImageAspectFlagBits get_image_aspects() const;
  const VkImageLayout get_image_layout() const;

  VkImageSubresourceRange get_subressource_range_cube() const;
  VkImageSubresourceRange get_subressource_range_face(uint32_t face) const;
  VkImageSubresourceRange get_subressource_range_face(uint32_t baseLayer = 0, uint32_t layers = 6) const;

 private:
  std::shared_ptr<Anvil::Image> get_flattened(bool nice = false);

  VkFormat compute_image_format_;
  glm::ivec2 render_size_;
  Usages usage_;

  std::weak_ptr<Anvil::SGPUDevice> device_ptr_;
  std::shared_ptr<Anvil::Image> images_;

  VkFormat image_format_;
  VkImageUsageFlags image_usage_;
  VkImageAspectFlagBits image_aspects_;
  VkImageLayout image_layout_;

  Anvil::MemoryFeatureFlags memory_features_;
  VkImageTiling tiling_;

  // views
  std::shared_ptr<Anvil::ImageView> view_cube_map_;
  std::shared_ptr<Anvil::ImageView> view_texture_array_;
};
}  // namespace quavis

#endif  // QUAVIS_RENDER_CUBE_IMAGES
