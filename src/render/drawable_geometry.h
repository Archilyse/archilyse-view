#ifndef QUAVIS_RENDER_DRAWABLE_GEOMETRY
#define QUAVIS_RENDER_DRAWABLE_GEOMETRY

#include <vector>

#include <glm/glm.hpp>

#include "./anvil.h"

namespace quavis {
/// Something that can be drawn. A set of vertex position/index based triangles with an additional 4 floats per vertex that can be used by the material
class DrawableGeometry {
 public:
  /// Slow constructor(vector data is copied) that supports glm types
  DrawableGeometry(std::vector<glm::vec3> &&positions, std::vector<glm::vec4> &&per_vertex_data, std::vector<uint32_t> &&indices);
  /// Fast (move) constructor. needs positions.size() % 3 and per_vertex_data.size() % 4
  DrawableGeometry(std::vector<float> &&positions, std::vector<float> &&per_vertex_data, std::vector<uint32_t> &&indices);
  /// creates a unit cube (coordinates from -0.5 to +0.5)
  static std::shared_ptr<DrawableGeometry> create_unit_cube();

  /// sends data to the GPU
  void prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt);

  /// draws the triangles
  void draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer);

 private:
  std::vector<float> positions_;
  std::vector<float> per_vertex_data_;
  std::vector<uint32_t> indicies_;

  // gpu data
  std::vector<std::shared_ptr<Anvil::Buffer>> vbos_;
  std::vector<VkDeviceSize> vbos_offsets_;
  std::shared_ptr<Anvil::Buffer> indicies_vbo_;

  // helper functions
  /// Creates a new buffer and download data. Because data is directly downlaoded GPU memory has to be  allocated immediately and can not be shared
  /// with other buffers
  template <class T>
  std::shared_ptr<Anvil::Buffer> download_to_new_vbo(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::MemoryAllocator> allocator,
                                                     const std::vector<T> &data, VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) const
  {
    auto result =
      Anvil::Buffer::create_nonsparse(device_ptr, data.size() * sizeof(data[0]), Anvil::QUEUE_FAMILY_GRAPHICS_BIT, VK_SHARING_MODE_EXCLUSIVE, usage);

    allocator->add_buffer(result, 0);

    result->write(0, /* start_offset */
                  data.size() * sizeof(data[0]), data.data());

    return result;
  }
};
}  // namespace quavis

#endif