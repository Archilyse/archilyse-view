#ifndef QUAVIS_RENDER_MATERIALS_BASE
#define QUAVIS_RENDER_MATERIALS_BASE

#include <string>

#include "../anvil.h"

namespace quavis {
/// the most simple base material use by the renderer, it supports output to the 6 layers of a cube map at once.
class MaterialBase {
 public:
  /// called once before first time of drawing
  virtual void prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt){};

  /// name of the material, used to reuse the same pipeline for materials with the same name and group scene_objects
  virtual const std::string get_name() { return "default"; };

  /// called immediately before the material is used in a rendering call
  virtual void use(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer){};

  void set_pipline(const Anvil::GraphicsPipelineID pipeline) { pipeline_ = pipeline; }

  /// called once during pipeline creation (remember there is only one pipeline for one given material name), used to configure pipeline, think of
  /// this function as static
  virtual void add_per_material_description(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                            std::shared_ptr<Anvil::GraphicsPipelineManager> &gfx_pipeline_manager_ptr_,
                                            std::shared_ptr<Anvil::DescriptorSetGroup> desc, Anvil::PipelineID pipline);

  /// called before each drawing for each material instance to set resources to the pipline (texturesampler,...)
  virtual void set_material_properties(std::weak_ptr<Anvil::SGPUDevice> device_pt, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                       std::shared_ptr<Anvil::PipelineLayout> pipeline_layout);

 private:
  Anvil::GraphicsPipelineID pipeline_;

 public:
  virtual const char *get_shader_src_fragment();                ///< source code or nullptr if stage is not used
  virtual const char *get_shader_src_geometry();                ///< source code or nullptr if stage is not used
  virtual const char *get_shader_src_tess_control();            ///< source code or nullptr if stage is not used
  virtual const char *get_shader_src_tess_evaluation_shader();  ///< source code or nullptr if stage is not used
  virtual const char *get_shader_src_vertex();                  ///< source code or nullptr if stage is not used
};
};  // namespace quavis

#endif
