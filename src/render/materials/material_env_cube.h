#ifndef QUAVIS_RENDER_MATERIALS_ENV_CUBE
#define QUAVIS_RENDER_MATERIALS_ENV_CUBE

#include <string>

#include "./material_base.h"

#include "../cube_images.h"

namespace quavis {
/// a material that uses vertex_data to lookup output color in a cube map
class MaterialEnvCube : public MaterialBase {
 public:
  /// the cube_image that should be used
  MaterialEnvCube(std::shared_ptr<CubeImages> &cube_iamge);

  virtual const std::string get_name() override { return "envCube"; };

  virtual void prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt) override;

  /// called immediately before the material is used in a rendering call
  virtual void use(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer) override;

  /// prepare the pipline to hold an image cubemap sampler
  virtual void add_per_material_description(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                            std::shared_ptr<Anvil::GraphicsPipelineManager> &gfx_pipeline_manager_ptr_,
                                            std::shared_ptr<Anvil::DescriptorSetGroup> desc_group, Anvil::PipelineID pipline) override;

  /// set pipline to point to the instance cube_image
  virtual void set_material_properties(std::weak_ptr<Anvil::SGPUDevice> device_pt, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                       std::shared_ptr<Anvil::PipelineLayout> pipeline_layout) override;

  // Shader Sources
 public:
  virtual const char *get_shader_src_fragment() override;

 private:
  std::shared_ptr<Anvil::DescriptorSet> view_set_;
  std::shared_ptr<CubeImages> cube_image_;
};
};  // namespace quavis

#endif