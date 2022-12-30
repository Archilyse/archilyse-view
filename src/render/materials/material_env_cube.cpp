#include "material_env_cube.h"

quavis::MaterialEnvCube::MaterialEnvCube(std::shared_ptr<CubeImages> &cube_image)
  : cube_image_(cube_image)
{
}

void quavis::MaterialEnvCube::prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt) {}

void quavis::MaterialEnvCube::use(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer) {}

void quavis::MaterialEnvCube::add_per_material_description(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                                           std::shared_ptr<Anvil::GraphicsPipelineManager> &gfx_pipeline_manager_ptr_,
                                                           std::shared_ptr<Anvil::DescriptorSetGroup> desc_group, Anvil::PipelineID pipline)
{
  // add texture
  desc_group->add_binding(2, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, nullptr);

  view_set_ = desc_group->get_descriptor_set(2);

  auto sampler = Anvil::Sampler::create(device_pt, VkFilter::VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
                                        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0, 0, false,
                                        VK_COMPARE_OP_NEVER, 0.0, 0.0, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, true);

  view_set_->set_binding_item(
    0, Anvil::DescriptorSet::CombinedImageSamplerBindingElement(cube_image_->get_image_layout(), cube_image_->get_view_cubemap(), sampler));
  view_set_->bake();
}

void quavis::MaterialEnvCube::set_material_properties(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                                      std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                                      std::shared_ptr<Anvil::PipelineLayout> pipeline_layout)
{
  command_buffer->record_bind_descriptor_sets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 2, 1, &view_set_, 0, nullptr);
}

const char *quavis::MaterialEnvCube::get_shader_src_fragment()
{
  return R"(
  #version 450


  // SET: 0  World Static variables (lights,...)
  // layout(set = 0, binding = 0) uniform WorldProp {
  
  // } worldProp;

  // SET: 1  Per View Matrices
  layout(set = 1, binding = 0) uniform WiewProp {
      mat4 view_projection_matrix[6];
      vec4 position;
  } viewProp;

  // SET: 2  Shader Config (setting for this material for all objs)
  layout(set = 2, binding = 0) uniform samplerCube envMap;

  // SET: 3  Per Object Material Parameters
  // layout(set = 3, binding = 0) uniform MaterialProp {
    
  // } materialProp;

  // push_constants Per Object Parameters 
  layout(push_constant) uniform ObjProp {
    mat4 model_mat;
  } objProp;



  layout(location = 0) out vec4 fColor;

  in layout(location = 0) fData
  {
     vec4 color;
     vec3 worldPos;
  };

  void main() {
    fColor = texture(envMap, normalize(color.xyz - vec3(0.5, 0.5, 0.5)));
    fColor.a = distance(worldPos, viewProp.position.xyz);
  }
)";
}