#ifndef QUAVIS_RENDER_RENDER
#define QUAVIS_RENDER_RENDER

#include <array>
#include <future>
#include <memory>

#include <wrappers/device.h>
#include <glm/glm.hpp>

#include "../logger.h"
#include "./anvil.h"
#include "./cube_images.h"
#include "./observation.h"
#include "./scene_object.h"

namespace quavis {

/// Render holds a list of scene objects, materials (indirectly) and observations and allows to render 6 sided cube maps of the scene from all
/// observation points
class Render : UseLogger {
 public:
  /// creates a renderer that randers each cube map size with the given resolution (x=width, y=height). It creates the inits the vulkan device with
  /// number (vulkan_device_idx).
  Render(const glm::ivec2 &render_dim, uint32_t vulkan_device_idx = 0);

  /// adds a new scene object to the world.
  void add_static_scene_object(std::shared_ptr<SceneObject> sceneObject);
  /// adds all observation points (move)
  void add_observations(std::vector<Observation> &&observations);

  /// returns the the cube image that is the target for all rendering. It will be reused for all draw calls.
  std::shared_ptr<CubeImages> get_color_cube();
  /// the depth and stencil buffer
  std::shared_ptr<CubeImages> get_depth_cube();

  /// draws the scene from the observation point observation_idx
  std::shared_ptr<CubeImages> draw(size_t observation_idx);

  /// returns the used vulkan device
  std::weak_ptr<Anvil::SGPUDevice> get_device() { return device_ptr_; }

  const glm::vec2 get_render_size() const { return render_size_; }
  /// number of observations
  const size_t observations_size() const { return observations_.size(); }

 private:
  /// Holds all the information needed for one kind of material (all material instances share the same pipeline)
  struct MaterialCache {
    std::vector<std::shared_ptr<SceneObject>> objects;
    Anvil::PipelineID pipeline;
    Anvil::SubPassID subpass;
    std::shared_ptr<Anvil::RenderPass> render_pass;

    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_fragment;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_geometry;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_tess_control;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_tess_evaluation_shader;
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_vertex;

    std::shared_ptr<Anvil::DescriptorSetGroup> descriptor_group;
  };

  void init_vulkan(uint32_t vulkan_device_idx);

  void create_images();
  void create_framebuffer();
  void create_render_pass();
  void create_base_pipeline();

  void create_world_ubo();

  void create_material_pipelines();
  MaterialCache create_pipeline_for_material(std::shared_ptr<MaterialBase> material);

  void create_static_object_buffers();
  void draw_static_objects();

  // void create_descriptors();

  void add_descriptor_layouts(std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr_, MaterialCache &pipeline,
                              std::shared_ptr<MaterialBase> material);
  void create_observations_ubo();
  std::shared_ptr<Anvil::Buffer> view_mat_ubo_;
  std::shared_ptr<Anvil::DescriptorSet> view_set_;
  std::vector<Observation> observations_;

 private:
  /// the data availble to the shader for each observation point
  struct SceneShaderData_data {
    std::array<glm::mat4, 6> projection_view_matrix;
    glm::vec4 position;
    glm::vec3 view_direction;
    float field_of_view;
  };

  /// create a data structure that is large enough to support the min offset of the device (minUniformBufferOffsetAlignment). \todo get from actual
  /// device
  template <class T, size_t padding>
  struct DataPadding : public T {
   public:
    static constexpr size_t get_padding_size() { return (padding - (sizeof(T) % padding)) % padding; }

   private:
    std::array<uint8_t, DataPadding::get_padding_size()> padding_;
  };

  typedef DataPadding<SceneShaderData_data, 256> SceneShaderData;

  // member values
  glm::ivec2 render_size_;

  bool dirty_scene_{true};
  bool dirty_observations_{true};

  std::vector<std::shared_ptr<SceneObject>> scene_objects_;
  // rendering
  std::shared_ptr<CubeImages> cube_images_color_;
  std::shared_ptr<CubeImages> cube_images_depth_;

  std::shared_ptr<Anvil::Framebuffer> framebuffer_;
  Anvil::FramebufferAttachmentID framebuffer_color_attachment_id_;

  // helper structures, need to be recomputed when dirty
  std::map<std::string, MaterialCache> material_cache;

  // Vulkan stuff
  std::weak_ptr<Anvil::SGPUDevice> device_ptr_;
  std::shared_ptr<Anvil::GraphicsPipelineManager> gfx_pipeline_manager_ptr_;
};
};  // namespace quavis

#endif