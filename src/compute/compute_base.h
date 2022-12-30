#ifndef QUAVIS_COMPUTE_COMPUTE_BASE
#define QUAVIS_COMPUTE_COMPUTE_BASE

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "../render/anvil.h"
#include "../render/cube_images.h"

namespace quavis {

/// the result of a computation , series of float values or an image
struct ComputeResult {
  std::vector<float> values;
  std::shared_ptr<ImageCPU> image;
};

class ComputeBase {
 public:
  virtual std::shared_ptr<ComputeResult> compute(std::shared_ptr<CubeImages> render_result, std::shared_ptr<CubeImages> depth) = 0;
};

/// description of each stage (compute shader invocation) of a GPU based computation
struct ComputeShaderStage {
  /** compute shader source code
   * compute shader have acces to set 0 and following bindings
   *  layout (binding = 0, rgba32f) uniform readonly image2DArray colorImage;
   *  // layout (binding = 1, rg32f) uniform readonly image2DArray depthImage; // reserved
   *  layout (binding = 2) buffer InputBuffer {} inputs;
   *  layout (binding = 3) buffer OutputBuffer {} outputs;
   *  layout(push_constant) uniform Parameters {} parameters;
   **/
  const char *shader_code;

  glm::ivec3 work_group_size;     ///< the work group size used to invoke compute shader
  size_t input_buffer_size  = 0;  ///< number of input bytes, this overlaps with output of previoues stage
  size_t output_buffer_size = 0;  ///< number of output bytes, this can be used as input in the next stage

  bool has_shader_parameters         = false;  ///< should accesses pushed paramters
  bool input_color_cube              = true;   ///< does the shader need access to the color_cube
  size_t retrieve_output_buffer_size = 0;      ///< number of bytes that should be uploaded from the outbuffer after ALL stages are done.
};

class ComputeBaseGPUImpl : public ComputeBase {
 public:
  virtual std::shared_ptr<ComputeResult> compute(std::shared_ptr<CubeImages> render_result, std::shared_ptr<CubeImages> depth) = 0;

 protected:
  ComputeBaseGPUImpl(std::weak_ptr<Anvil::SGPUDevice> device_ptr);

  void create_pipelines(const std::vector<ComputeShaderStage> &stages, uint32_t parameter_size);
  void create_buffers(const std::vector<ComputeShaderStage> &stages);

  std::shared_ptr<ComputeResult> compute_all_stages(const std::vector<ComputeShaderStage> &shader_stages_, std::shared_ptr<CubeImages> render_result,
                                                    std::shared_ptr<CubeImages> depth, const void *parameter_data, uint32_t parameter_size);

 private:
  std::shared_ptr<Anvil::Buffer> create_buffer(VkDeviceSize size, bool mapable) const;

  struct PipelineStage {
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader;
    Anvil::ComputePipelineID pipelineId;

    std::shared_ptr<Anvil::DescriptorSetGroup> descriptor_group;
    std::shared_ptr<Anvil::Buffer> input_buffer;
    std::shared_ptr<Anvil::Buffer> output_buffer;
  };

  std::weak_ptr<Anvil::SGPUDevice> device_ptr_;

  std::vector<PipelineStage> pipelines_;
};

/// Base class for all GPU based computations, when deriving specify the parameter struct, and make sure shader_stages_ is initialized in constructor.

template <class ComputeShaderParameterStruct>
class ComputeBaseGPU : public ComputeBaseGPUImpl {
 public:
  ComputeBaseGPU(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const std::vector<ComputeShaderStage> &&stages)
    : ComputeBaseGPUImpl{device_ptr}
    , shader_stages_{std::move(stages)}
  {
    create_pipelines(shader_stages_, sizeof(ComputeShaderParameterStruct));
    create_buffers(shader_stages_);
  }

  virtual std::shared_ptr<ComputeResult> compute(std::shared_ptr<CubeImages> render_result, std::shared_ptr<CubeImages> depth) override
  {
    const ComputeShaderParameterStruct p = get_parameter();
    return compute_all_stages(shader_stages_, render_result, depth, &p, sizeof(ComputeShaderParameterStruct));
  }

  std::shared_ptr<ComputeResult> compute(const ComputeShaderParameterStruct &compute_parameter, std::shared_ptr<CubeImages> render_result,
                                         std::shared_ptr<CubeImages> depth)
  {
    return compute_all_stages(shader_stages_, render_result, depth, &compute_parameter, sizeof(compute_parameter));
  }

  virtual const ComputeShaderParameterStruct get_parameter() { return ComputeShaderParameterStruct{}; }

 protected:
  const std::vector<ComputeShaderStage> shader_stages_;
};

}  // namespace quavis

#endif