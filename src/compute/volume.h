#ifndef QUAVIS_COMPUTE_VOLUME
#define QUAVIS_COMPUTE_VOLUME

#include "./compute_base.h"

namespace quavis {
struct ComputeVolumeParams {
  int width;
  int height;
  float r_max;
};

class ComputeVolume : public ComputeBaseGPU<ComputeVolumeParams> {
 public:
  ComputeVolume(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2& image_dim);

  virtual const ComputeVolumeParams get_parameter() override;

 private:
  const glm::ivec2 image_dim_;

  std::vector<ComputeShaderStage> create_compute_stages(const glm::ivec2& image_dim);
};
}  // namespace quavis

#endif