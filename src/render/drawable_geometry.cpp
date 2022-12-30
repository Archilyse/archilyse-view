#include "drawable_geometry.h"

quavis::DrawableGeometry::DrawableGeometry(std::vector<glm::vec3>&& positions, std::vector<glm::vec4>&& per_vertex_data,
                                           std::vector<uint32_t>&& indicies)
  : indicies_(std::move(indicies))
{
  assert(indicies.size() % 3 == 0);                    // because we draw triangles
  assert(positions.size() == per_vertex_data.size());  // because we need same amount of vertex data

  for (const auto& p : positions) {
    positions_.push_back(p.x);
    positions_.push_back(p.y);
    positions_.push_back(p.z);
  }

  for (const auto& p : per_vertex_data) {
    per_vertex_data_.push_back(p.x);
    per_vertex_data_.push_back(p.y);
    per_vertex_data_.push_back(p.z);
    per_vertex_data_.push_back(p.w);
  }
}

quavis::DrawableGeometry::DrawableGeometry(std::vector<float>&& positions, std::vector<float>&& per_vertex_data, std::vector<uint32_t>&& indicies)
  : positions_(std::move(positions))
  , per_vertex_data_(std::move(per_vertex_data))
  , indicies_(std::move(indicies))
{
  assert(positions.size() % 3 == 0);                           // because we need 3 coordiantes
  assert(per_vertex_data.size() % 4 == 0);                     // because we need 4 coordiantes
  assert(indicies.size() % 3 == 0);                            // because we draw triangles
  assert(positions.size() / 3 == per_vertex_data.size() % 4);  // because we need same amount of vertex data
}
#if 0  // first version not optimized
void quavis::DrawableGeometry::prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr)
{
	auto allocator{ Anvil::MemoryAllocator::create_vma(device_ptr) };

	// create vbos and download data
	vbos_.clear();
	vbos_offsets_.clear();

	vbos_.push_back(download_to_new_vbo(device_ptr, allocator, positions_));
	vbos_offsets_.push_back(0);
	vbos_.push_back(download_to_new_vbo(device_ptr, allocator, per_vertex_data_));
	vbos_offsets_.push_back(0);

	indicies_vbo_ = download_to_new_vbo(device_ptr, allocator, indicies_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}
#else
void quavis::DrawableGeometry::prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr)
{
  auto allocator{Anvil::MemoryAllocator::create_vma(device_ptr)};

  // vertex buffer data
  VkDeviceSize size_data = 0;
  vbos_offsets_.push_back(size_data);
  size_data += positions_.size() * sizeof(positions_[0]);

  vbos_offsets_.push_back(size_data);
  size_data += per_vertex_data_.size() * sizeof(per_vertex_data_[0]);

  auto vbo = Anvil::Buffer::create_nonsparse(device_ptr, size_data, Anvil::QUEUE_FAMILY_GRAPHICS_BIT, VK_SHARING_MODE_EXCLUSIVE,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  allocator->add_buffer(vbo, 0);

  // now push it twice (for position and data)
  vbos_.push_back(vbo);
  vbos_.push_back(vbo);

  assert(vbos_offsets_.size() == vbos_.size());

  // create ibo
  // this is the first write so here memory is really reserved. other writes behind this
  indicies_vbo_ = download_to_new_vbo(device_ptr, allocator, indicies_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  vbo->write(vbos_offsets_[0], /* start_offset */
             positions_.size() * sizeof(positions_[0]), positions_.data());

  vbo->write(vbos_offsets_[1], /* start_offset */
             per_vertex_data_.size() * sizeof(per_vertex_data_[0]), per_vertex_data_.data());
}

#endif

void quavis::DrawableGeometry::draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer)
{
  command_buffer->record_bind_vertex_buffers(0, static_cast<uint32_t>(vbos_.size()), vbos_.data(), vbos_offsets_.data());

  command_buffer->record_bind_index_buffer(indicies_vbo_, 0, VK_INDEX_TYPE_UINT32);
  command_buffer->record_draw_indexed(static_cast<uint32_t>(indicies_.size()), 1, 0, 0, 0);
}

std::shared_ptr<quavis::DrawableGeometry> quavis::DrawableGeometry::create_unit_cube()
{
  // clang-format off
	std::vector<float> pos{
		// front
		-0.5, -0.5,  -0.5,
		0.5, -0.5,  -0.5,
		0.5,  0.5,  -0.5,
		-0.5,  0.5,  -0.5,
		// back
		-0.5, -0.5,  0.5,
		0.5, -0.5,  0.5,
		0.5,  0.5,  0.5,
		-0.5,  0.5, 0.5
	};
	std::vector<float> color{
		// front colors
		0.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		1.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		// back colors 1.0,
		0.0, 0.0, 1.0, 1.0,
		1.0, 0.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		0.0, 1.0, 1.0, 1.0
	};
	std::vector<uint32_t> indx{
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// bottom
		4, 0, 3,
		3, 7, 4,
		// left
		4, 5, 1,
		1, 0, 4,
		// right
		3, 2, 6,
		6, 7, 3
	};
  // clang-format on

  return std::make_shared<quavis::DrawableGeometry>(std::move(pos), std::move(color), std::move(indx));
}
