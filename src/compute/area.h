#ifndef QUAVIS_COMPUTE_AREA
#define QUAVIS_COMPUTE_AREA

#include "./compute_base.h"

namespace quavis {
struct ComputeAreaParams {
  int width;
  int height;
  float r_max;
};

class ComputeArea : public ComputeBaseGPU<ComputeAreaParams> {
 public:
  ComputeArea(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2& image_dim);

  virtual const ComputeAreaParams get_parameter() override;

 private:
  const glm::ivec2 image_dim_;

  std::vector<ComputeShaderStage> create_compute_stages(const glm::ivec2& image_dim);
};
}  // namespace quavis

#endif