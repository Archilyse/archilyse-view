#include "material_base.h"

void quavis::MaterialBase::add_per_material_description(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                                        std::shared_ptr<Anvil::GraphicsPipelineManager> &gfx_pipeline_manager_ptr_,
                                                        std::shared_ptr<Anvil::DescriptorSetGroup> desc_group, Anvil::PipelineID pipline)
{
}

void quavis::MaterialBase::set_material_properties(std::weak_ptr<Anvil::SGPUDevice> device_pt,
                                                   std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer,
                                                   std::shared_ptr<Anvil::PipelineLayout> pipeline_layout)
{
}

const char *quavis::MaterialBase::get_shader_src_fragment()
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
      vec3 view_direction;
      float field_of_view;
  } viewProp;

  // SET: 2  Shader Config (setting for this material for all objs)
  // layout(set = 2, binding = 0) uniform ShaderProp {

  // } shaderProp;

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
    fColor = vec4(color.xyz, distance(worldPos, viewProp.position.xyz));
  }
)";
}

const char *quavis::MaterialBase::get_shader_src_geometry()
{
  return R"(
  #version 450

  layout(triangles) in;
  layout(triangle_strip, max_vertices = 18) out;

  // SET: 0  World Static variables (lights,...)
  // layout(set = 0, binding = 0) uniform WorldProp {
  
  // } worldProp;

  // SET: 1  Per View Matrices
  layout(set = 1, binding = 0) uniform WiewProp {
      mat4 view_projection_matrix[6];
      vec4 position;
      vec3 view_direction;
      float field_of_view;
  } viewProp;

  // SET: 2  Shader Config (setting for this material for all objs)
  // layout(set = 2, binding = 0) uniform ShaderProp {

  // } shaderProp;

  // SET: 3  Per Object Material Parameters
  // layout(set = 3, binding = 0) uniform MaterialProp {
    
  // } materialProp;


  // push_constants Per Object Parameters 
  layout(push_constant) uniform ObjProp {
    mat4 model_mat;
  } objProp;



  in layout(location = 0) vData
  {
    vec4 color;
    vec3 worldPos;
  } vertices[];

  out layout(location = 0) fData
  {
    vec4 color;
    vec3 worldPos;
  } frag;

  void main() {
    for(int layer = 0; layer < 6; ++layer) {
      gl_Layer = layer;
      for(int i = 0; i < gl_in.length(); ++i) {
        frag.color = vertices[i].color;
        frag.worldPos = vertices[i].worldPos;
        // frag = vertices[i];
        gl_Position = viewProp.view_projection_matrix[layer] * gl_in[i].gl_Position;
        EmitVertex();
      }
      EndPrimitive();
    }
  }
)";
}

const char *quavis::MaterialBase::get_shader_src_tess_control()
{
  return nullptr;
}
const char *quavis::MaterialBase::get_shader_src_tess_evaluation_shader()
{
  return nullptr;
}

const char *quavis::MaterialBase::get_shader_src_vertex()
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
      vec3 view_direction;
      float field_of_view;
  } viewProp;

  // SET: 2  Shader Config (setting for this material for all objs)
  // layout(set = 2, binding = 0) uniform ShaderProp {

  // } shaderProp;

  // SET: 3  Per Object Material Parameters
  // layout(set = 3, binding = 0) uniform MaterialProp {
    
  // } materialProp;

  // push_constants Per Object Parameters 
  layout(push_constant) uniform ObjProp {
    mat4 model_mat;
  } objProp;

  
  layout(location = 0) in vec3 inPosition;
  layout(location = 1) in vec4 inColor;
  
  out layout(location = 0) vData
  {
    vec4 color;
    vec3 worldPos;
  };

  void main() {
    gl_Position =  objProp.model_mat * vec4(inPosition, 1.0);
    color = inColor;
    worldPos = gl_Position.xyz;
  }
)";
}