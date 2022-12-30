#define NOMINMAX
#include "compute_base.h"
#include "../utils/shader_loader.h"
#include <iostream>

using namespace quavis;

ComputeBaseGPUImpl::ComputeBaseGPUImpl(std::weak_ptr<Anvil::SGPUDevice> device_ptr)
  : device_ptr_{device_ptr}
{
}

void ComputeBaseGPUImpl::create_pipelines(const std::vector<ComputeShaderStage> &stages, uint32_t parameter_size)
{
  auto device{device_ptr_.lock()};

  auto pipeline_manager{device->get_compute_pipeline_manager()};

  for (const auto &stage : stages) {
    PipelineStage pipeline;

    pipeline.shader = ShaderLoader::create_shader_entry(device_ptr_, stage.shader_code, Anvil::ShaderStage::SHADER_STAGE_COMPUTE);

    pipeline_manager->add_regular_pipeline(false, false, *pipeline.shader, &pipeline.pipelineId);

    // configure pipeline
    if (stage.has_shader_parameters) {
      pipeline_manager->attach_push_constant_range_to_pipeline(pipeline.pipelineId, 0, parameter_size, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    pipeline.descriptor_group = Anvil::DescriptorSetGroup::create(device_ptr_, false, 1);

    if (stage.input_color_cube) {
      pipeline.descriptor_group->add_binding(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
    }

    // TODO
    // 		if (stage.input_depth_cube) {
    // 			pipeline.descriptor_group->add_binding(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
    // 		}

    if (stage.input_buffer_size) {
      pipeline.descriptor_group->add_binding(0, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
    }
    if (stage.output_buffer_size) {
      pipeline.descriptor_group->add_binding(0, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr);
    }

    pipeline_manager->set_pipeline_dsg(pipeline.pipelineId, pipeline.descriptor_group);

    pipelines_.push_back(std::move(pipeline));
  }
}

std::shared_ptr<ComputeResult> ComputeBaseGPUImpl::compute_all_stages(const std::vector<ComputeShaderStage> &stages,
                                                                      std::shared_ptr<CubeImages> render_result, std::shared_ptr<CubeImages> depth,
                                                                      const void *parameter_data, uint32_t parameter_size)
{
  auto result = std::make_shared<ComputeResult>();
  if (stages.size() == 0) {
    return result;
  }

  auto device{device_ptr_.lock()};
  auto pipeline_manager{device->get_compute_pipeline_manager()};
  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};
  auto queue = device->get_universal_queue(0);

  auto command_buffer{command_pool->alloc_primary_level_command_buffer()};

  command_buffer->start_recording(true, false);

  auto colorImage = render_result->get_storage_image_view(command_buffer, queue);

  auto colorBinding = Anvil::DescriptorSet::StorageImageBindingElement(VK_IMAGE_LAYOUT_GENERAL, colorImage);

  //  TODO
  // 	auto depthImage = depth->get_storage_image_view(command_buffer);
  // 	auto depthLayout = depth->get_image_layout();
  // 	auto depthBinding = Anvil::DescriptorSet::StorageImageBindingElement(colorLayout, depthImage);

  // do we need to download something?
  if (stages.front().input_buffer_size > 0) {
    // TODO
    assert(!"not implemented yet");
  }

  for (size_t i = 0; i < stages.size(); i++) {
    const auto &stage    = stages[i];
    const auto &pipeline = pipelines_[i];
    auto pipeline_layout = pipeline_manager->get_compute_pipeline_layout(pipeline.pipelineId);

    auto bindings = pipeline.descriptor_group->get_descriptor_set(0);

    if (stage.input_color_cube) {
      bindings->set_binding_item(0, colorBinding);
    }

    // TODO
    // 		if (stage.input_depth_cube) {
    // 			bindings->set_binding_item(1, depthBinding);
    // 		}
    if (stage.input_buffer_size) {
      bindings->set_binding_item(2, Anvil::DescriptorSet::StorageBufferBindingElement(pipeline.input_buffer));
    }
    if (stage.output_buffer_size) {
      bindings->set_binding_item(3, Anvil::DescriptorSet::StorageBufferBindingElement(pipeline.output_buffer));
    }

    command_buffer->record_bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipelineId);
    if (stage.has_shader_parameters) {
      command_buffer->record_push_constants(pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, parameter_size, parameter_data);
    }

    command_buffer->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &bindings, 0, nullptr);

    command_buffer->record_dispatch(stage.work_group_size.x, stage.work_group_size.y, stage.work_group_size.z);
    auto outputBufferBarrier = Anvil::BufferBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, queue->get_queue_family_index(),
                                                    queue->get_queue_family_index(), pipeline.output_buffer, 0, stage.output_buffer_size);

    auto dstAccess = VK_ACCESS_SHADER_READ_BIT;
    command_buffer->record_pipeline_barrier(VK_ACCESS_SHADER_WRITE_BIT, dstAccess, false, 0, nullptr, 1, &outputBufferBarrier, 0, nullptr);
  }

  command_buffer->stop_recording();
  // submit compute
  queue->submit_command_buffer(command_buffer, true);

  // retrieve whatever we need
  for (size_t i = 0; i < stages.size(); i++) {
    const auto &stage    = stages[i];
    const auto &pipeline = pipelines_[i];

    if (stage.retrieve_output_buffer_size) {
      // output is casted to ComputeResult.values
      assert(stage.retrieve_output_buffer_size % sizeof(result->values.front()) == 0);

      auto size    = stage.retrieve_output_buffer_size / sizeof(result->values.front());
      auto oldSize = result->values.size();
      auto newSize = oldSize + size;
      result->values.resize(newSize);

      pipeline.output_buffer->read(0, stage.retrieve_output_buffer_size, &result->values[oldSize]);
    }
  }

  return result;
}

std::shared_ptr<Anvil::Buffer> quavis::ComputeBaseGPUImpl::create_buffer(VkDeviceSize size, bool mapable) const
{
  // TODO why does this assert in ANVIL but does work in release build (i.e ignoring assert)
  // 	return  Anvil::Buffer::create_nonsparse(device_ptr_, size, Anvil::QUEUE_FAMILY_COMPUTE_BIT, VK_SHARING_MODE_EXCLUSIVE,
  // VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, mapable ? Anvil::MemoryFeatureFlagBits::MEMORY_FEATURE_FLAG_MAPPABLE : 0, nullptr);
  return Anvil::Buffer::create_nonsparse(device_ptr_, size, Anvil::QUEUE_FAMILY_COMPUTE_BIT, VK_SHARING_MODE_EXCLUSIVE,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void quavis::ComputeBaseGPUImpl::create_buffers(const std::vector<ComputeShaderStage> &stages)
{
  if (stages.size() == 0) {
    return;
  }

  auto allocator{Anvil::MemoryAllocator::create_oneshot(device_ptr_)};

  // do we need an import buffer?
  if (stages.front().input_buffer_size > 0) {
    allocator->add_buffer(pipelines_.front().input_buffer = create_buffer(stages.front().input_buffer_size, false), 0);
  }

  for (size_t i = 0; i < stages.size() - 1; i++) {
    // reserve in between buffers
    auto size = std::max(stages[i].output_buffer_size, stages[i + 1].input_buffer_size);
    allocator->add_buffer(
      pipelines_[i].output_buffer = pipelines_[i + 1].input_buffer = create_buffer(size, stages[i].retrieve_output_buffer_size > 0), 0);
  }

  // do we need an output buffer?
  if (stages.back().output_buffer_size > 0) {
    allocator->add_buffer(
      pipelines_.back().output_buffer = create_buffer(stages.back().output_buffer_size, stages.back().retrieve_output_buffer_size > 0), 0);
  }

  allocator->bake();
}
