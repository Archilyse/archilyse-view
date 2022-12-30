#include "render.h"

#include <functional>

#include <glm/gtc/matrix_transform.hpp>

#include "../app_config.h"
#include "../utils/shader_loader.h"
#include "./anvil.h"

using namespace std;

namespace quavis {

// local non member function

VkBool32 on_validation_callback(VkDebugReportFlagsEXT message_flags, VkDebugReportObjectTypeEXT object_type, const char *layer_prefix,
                                const char *message, void *user_arg)
{
  auto logger = spdlog::get("default");

  if ((message_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
    logger->info(message);
  else if ((message_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
    logger->warn(message);
  else if ((message_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0)
    logger->warn(message);
  else if ((message_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
    logger->error(message);
  else if ((message_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0)
    logger->debug(message);

  return false;
}

// Render function

Render::Render(const glm::ivec2 &render_dim, uint32_t vulkan_device_idx)
{
  render_size_ = render_dim;

  init_vulkan(vulkan_device_idx);
  create_framebuffer();
  create_render_pass();
  create_base_pipeline();
  create_images();

  dirty_scene_        = true;
  dirty_observations_ = true;
}

void Render::add_static_scene_object(std::shared_ptr<SceneObject> sceneObject)
{
  dirty_scene_ = true;

  scene_objects_.push_back(sceneObject);
}

void Render::add_observations(std::vector<Observation> &&observations)
{
  dirty_observations_ = true;
  observations_       = std::move(observations);
}

std::shared_ptr<CubeImages> Render::get_color_cube()
{
  return cube_images_color_;
}

std::shared_ptr<CubeImages> Render::get_depth_cube()
{
  return cube_images_depth_;
}

std::shared_ptr<CubeImages> Render::draw(size_t observation_idx)
{
  // create_framebuffer();
  // create_images();

  if (dirty_scene_) {
    create_static_object_buffers();
    create_material_pipelines();
    dirty_scene_ = false;
  }

  if (dirty_observations_) {
    create_observations_ubo();
    dirty_observations_ = false;
  }

  // select right observation
  view_set_->set_binding_item(
    0, Anvil::DescriptorSet::UniformBufferBindingElement(view_mat_ubo_, observation_idx * sizeof(SceneShaderData), sizeof(SceneShaderData)));
  view_set_->bake();

  draw_static_objects();

  return get_color_cube();
}

void Render::init_vulkan(uint32_t vulkan_device_idx)
{
  // decide if do validation
  Anvil::PFNINSTANCEDEBUGCALLBACKPROC logging_function{nullptr};
  if (ENABLE_VALIDATION) {
    logging_function = on_validation_callback;
  }

  /* Create a Vulkan instance */
  auto instance_ptr = Anvil::Instance::create(APP_NAME,                   /* app_name */
                                              APP_NAME,                   /* engine_name */
                                              logging_function, nullptr); /* validation_proc_user_arg */

  if (instance_ptr == nullptr || instance_ptr->get_n_physical_devices() == 0) {
    throw std::runtime_error("No GPU found");
  }

  auto dev_count = instance_ptr->get_n_physical_devices();

  if (vulkan_device_idx >= dev_count) {
    throw std::runtime_error("No GPU found for given device index");
  }

  auto physical_device_ptr = instance_ptr->get_physical_device(vulkan_device_idx);

  std::string name(physical_device_ptr.lock()->get_device_properties().deviceName);
  logger_->info("Using GPU: {}", name);

  // std::string name1(physical_device_ptr1.lock()->get_device_properties().deviceName);

  /* Create a Vulkan device */
  device_ptr_ = Anvil::SGPUDevice::create(physical_device_ptr, Anvil::DeviceExtensionConfiguration(), std::vector<std::string>(), /* layers */
                                          false, /* transient_command_buffer_allocs_only */
                                          false);

  auto device = device_ptr_.lock();

  gfx_pipeline_manager_ptr_ = {device->get_graphics_pipeline_manager()};
}

void Render::create_images()
{
  cube_images_color_ = std::make_shared<CubeImages>(device_ptr_, render_size_, CubeImages::USAGE_COLOR_ATTACHMENT);
  cube_images_depth_ = std::make_shared<CubeImages>(device_ptr_, render_size_, CubeImages::USAGE_DEPTH_ATTACHMENT);

  // cube_images_color_->clear_images( { 0.0f, 0.8f, 0.3f, 1.0f } );
  // cube_images_depth_->clear_images( {-1.0f, -1.0f, -1.0f, -1.0f });
  framebuffer_->add_attachment(cube_images_color_->get_view_cubemap(), &framebuffer_color_attachment_id_);
  framebuffer_->add_attachment(cube_images_depth_->get_view_cubemap(), nullptr);
}

void Render::create_framebuffer()
{
  framebuffer_ = Anvil::Framebuffer::create(device_ptr_, render_size_.x, render_size_.y, 6);
  framebuffer_->set_name("Framebuffer cube map target");
}

void Render::create_render_pass() {}

void Render::create_world_ubo() {}

void Render::create_base_pipeline()
{
  //// For now i could not figure out how todo reuse pipelines in anvil
}

void Render::create_material_pipelines()
{
  material_cache.clear();
  for (const auto &obj : scene_objects_) {
    auto material = obj->get_material();
    if (material_cache.find(material->get_name()) == material_cache.end()) {
      // create a new Material
      MaterialCache cache{create_pipeline_for_material(material)};
      material_cache[material->get_name()] = cache;
    }

    material_cache[material->get_name()].objects.push_back(obj);
  }
}

Render::MaterialCache Render::create_pipeline_for_material(std::shared_ptr<MaterialBase> material)
{
  auto device{device_ptr_.lock()};

  MaterialCache res;

  res.render_pass   = Anvil::RenderPass::create(device_ptr_, nullptr);
  auto &render_pass = res.render_pass;

  Anvil::RenderPassAttachmentID color_attachemnt_id;
  render_pass->add_color_attachment(cube_images_color_->get_image_format(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                    VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                    false, /* may_alias */
                                    &color_attachemnt_id);

  Anvil::RenderPassAttachmentID depth_attachemnt_id;

  render_pass->add_depth_stencil_attachment(cube_images_depth_->get_image_format(), VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                            VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, /* stencil_load_op  */
                                            VK_ATTACHMENT_STORE_OP_DONT_CARE,                              /* stencil_store_op */
                                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                            false, /* may_alias */
                                            &depth_attachemnt_id);

  // render_pass->add_depth_stencil_attachment()

  res.shader_fragment = ShaderLoader::create_shader_entry(device_ptr_, material->get_shader_src_fragment(), Anvil::SHADER_STAGE_FRAGMENT);
  res.shader_geometry = ShaderLoader::create_shader_entry(device_ptr_, material->get_shader_src_geometry(), Anvil::SHADER_STAGE_GEOMETRY);
  res.shader_tess_control =
    ShaderLoader::create_shader_entry(device_ptr_, material->get_shader_src_tess_control(), Anvil::SHADER_STAGE_TESSELLATION_CONTROL);
  res.shader_tess_evaluation_shader =
    ShaderLoader::create_shader_entry(device_ptr_, material->get_shader_src_tess_evaluation_shader(), Anvil::SHADER_STAGE_TESSELLATION_EVALUATION);
  res.shader_vertex = ShaderLoader::create_shader_entry(device_ptr_, material->get_shader_src_vertex(), Anvil::SHADER_STAGE_VERTEX);

  render_pass->add_subpass(*res.shader_fragment, *res.shader_geometry, *res.shader_tess_control, *res.shader_tess_evaluation_shader,
                           *res.shader_vertex, &res.subpass);

  render_pass->get_subpass_graphics_pipeline_id(res.subpass, &res.pipeline);

  // config this pipeline
  gfx_pipeline_manager_ptr_->set_viewport_properties(res.pipeline, 0, 0.0f, 0.0f, static_cast<float>(render_size_.x),
                                                     static_cast<float>(render_size_.y), 0.0f, 1.0f);
  gfx_pipeline_manager_ptr_->set_scissor_box_properties(res.pipeline, 0, 0, 0, render_size_.x, render_size_.y);
  gfx_pipeline_manager_ptr_->toggle_depth_test(res.pipeline, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  gfx_pipeline_manager_ptr_->toggle_depth_writes(res.pipeline, true); /* should_enable */
  gfx_pipeline_manager_ptr_->set_rasterization_properties(res.pipeline, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                          1.0f); /* line_width */

  // vertex positions
  gfx_pipeline_manager_ptr_->add_vertex_attribute(res.pipeline,
                                                  0,                           // vertex input location
                                                  VK_FORMAT_R32G32B32_SFLOAT,  // format
                                                  0,                           /* offset_in_bytes */
                                                  sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX, 0);

  // vertex data
  gfx_pipeline_manager_ptr_->add_vertex_attribute(res.pipeline,
                                                  1,                              // vertex input location
                                                  VK_FORMAT_R32G32B32A32_SFLOAT,  // format
                                                  0,                              /* offset_in_bytes */
                                                  sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX, 1);

  // add descriptor layouts
  add_descriptor_layouts(gfx_pipeline_manager_ptr_, res, material);

  // configure the render sub pass
  render_pass->add_subpass_color_attachment(res.subpass, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, color_attachemnt_id, 0, nullptr);
  render_pass->add_subpass_depth_stencil_attachment(res.subpass, depth_attachemnt_id, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  //// gfx_pipeline_manager_ptr_->add_vertex_attribute();

  // auto renderpass_ptr = Anvil::RenderPass::create(device_ptr_, nullptr);

  return res;
}

void Render::add_descriptor_layouts(std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr_, MaterialCache &cache,
                                    std::shared_ptr<MaterialBase> material)
{
  cache.descriptor_group = Anvil::DescriptorSetGroup::create(device_ptr_, true, /* releaseable_sets */
                                                             3                  /* n_sets */
  );

  // SET: 0  World Static variables (lights,...)
  // not needed yet

  // set 1 Per View Matrices
  cache.descriptor_group->add_binding(1, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, nullptr);

  // SET: 2  Shader Config (setting for this material for all objs)
  material->add_per_material_description(device_ptr_, gfx_pipeline_manager_ptr_, cache.descriptor_group, cache.pipeline);

  // SET: 3  Per Object Material Parameters
  // material->add_per_objcet_descriptions(3, descriptor_group);

  // SPush constants offset 0  Per Object Parameters
  gfx_pipeline_manager_ptr_->attach_push_constant_range_to_pipeline(cache.pipeline, 0, sizeof(SceneObject::ObjectShaderData),
                                                                    VK_SHADER_STAGE_ALL_GRAPHICS);

  gfx_pipeline_manager_ptr_->set_pipeline_dsg(cache.pipeline, cache.descriptor_group);

  view_set_ = cache.descriptor_group->get_descriptor_set(1);

  // view_set_->set_binding_item(0, Anvil::DescriptorSet::UniformBufferBindingElement(view_mat_ubo_));
}

void Render::create_observations_ubo()
{
  // observation point
  std::vector<SceneShaderData> shader_data;
  auto projection = glm::perspective<float>(3.14159f / 2.0f, static_cast<float>(render_size_.x) / static_cast<float>(render_size_.y), 0.01f, 100001.0f);
  const glm::mat4 clip{-1.0f, 0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f, +0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f, 0.5f, 1.0f};

  projection = clip * projection;

  for (const auto &opoint : observations_) {
    auto opos = opoint.position;

    SceneShaderData sd;

    sd.position = glm::vec4(opos, 1.0f);

    sd.projection_view_matrix[0] = projection * glm::lookAt(opos, opos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sd.projection_view_matrix[1] = projection * glm::lookAt(opos, opos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sd.projection_view_matrix[2] = projection * glm::lookAt(opos, opos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sd.projection_view_matrix[3] = projection * glm::lookAt(opos, opos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    sd.projection_view_matrix[4] = projection * glm::lookAt(opos, opos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    sd.projection_view_matrix[5] = projection * glm::lookAt(opos, opos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    sd.view_direction = opoint.view_direction;
    sd.field_of_view = opoint.field_of_view;

    shader_data.push_back(sd);
  }

  auto allocator{Anvil::MemoryAllocator::create_oneshot(device_ptr_)};

  view_mat_ubo_ = Anvil::Buffer::create_nonsparse(device_ptr_, shader_data.size() * sizeof(shader_data[0]), Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                  VK_SHARING_MODE_EXCLUSIVE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  allocator->add_buffer(view_mat_ubo_, 0);

  view_mat_ubo_->write(0, /* start_offset */
                       shader_data.size() * sizeof(shader_data[0]), shader_data.data());
}

void Render::create_static_object_buffers()
{
  for (auto &obj : scene_objects_) {
    obj->prepare_for_draw(device_ptr_);
  }
}

void Render::draw_static_objects()
{
  auto device{device_ptr_.lock()};
  auto command_pool{device->get_command_pool(Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL)};
  auto queue = device->get_universal_queue(0);

  auto command_buffer{command_pool->alloc_primary_level_command_buffer()};

  // todo remove!
  // vkDeviceWaitIdle(device->get_device_vk());

  command_buffer->start_recording(false, false);

  cube_images_color_->prepare_for_render(command_buffer, queue);

  VkRect2D render_area;
  render_area.extent.height = render_size_.x;
  render_area.extent.width  = render_size_.y;
  render_area.offset.x      = 0;
  render_area.offset.y      = 0;

  VkClearValue cv_color;
  cv_color.color.float32[0] = 0.0f;
  cv_color.color.float32[1] = 0.0f;
  cv_color.color.float32[2] = 0.0f;
  cv_color.color.float32[3] = 0.0f;

  VkClearValue cv_depth;
  cv_depth.depthStencil.depth   = 1.0f;
  cv_depth.depthStencil.stencil = 1;
  std::vector<VkClearValue> cv{cv_color, cv_depth};

  command_buffer->record_begin_render_pass(static_cast<uint32_t>(cv.size()), /* in_n_clear_values */
                                           cv.data(), framebuffer_, render_area, material_cache.begin()->second.render_pass,
                                           VK_SUBPASS_CONTENTS_INLINE);

  // set_material_properties_world(command_buffer);

  for (const auto &it : material_cache) {
    const auto &pipeline = it.second;
    auto pipeline_layout = gfx_pipeline_manager_ptr_->get_graphics_pipeline_layout(pipeline.pipeline);

    command_buffer->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 1, 1, &view_set_, 0, nullptr);

    command_buffer->record_bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    //
    for (const auto &obj : pipeline.objects) {
      const auto &material = obj->get_material();

      assert(material->get_name() == it.first);  // make sure there was no material obj mess up
      material->set_material_properties(device_ptr_, command_buffer, pipeline_layout);

      // object properties (model matrix)
      command_buffer->record_push_constants(pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(obj->get_shader_data()),
                                            &obj->get_shader_data());
      obj->draw(device_ptr_, command_buffer);
    }
  }

  command_buffer->record_end_render_pass();
  command_buffer->stop_recording();

  queue->submit_command_buffer(command_buffer, true, nullptr);

  // vkDeviceWaitIdle(device->get_device_vk());
}

}  // namespace quavis
