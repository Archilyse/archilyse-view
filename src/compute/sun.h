#ifndef QUAVIS_COMPUTE_SUN
#define QUAVIS_COMPUTE_SUN

#include "./compute_base.h"

namespace quavis {
struct ComputeSunParams {
  int width;
  int height;
  float r_max;
  float sun_azimuth_rad; // 0 = north, 1/2pi = east, pi = south, 3/2pi = west
  float sun_altitude_rad; // 0 = horizon, 1/2pi = zenith
  float zenith_luminance;
};

class ComputeSun : public ComputeBaseGPU<ComputeSunParams> {
 public:
  ComputeSun(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2& image_dim, const bool use_v2);

  virtual const ComputeSunParams get_parameter() override;

 private:
  const glm::ivec2 image_dim_;

  std::vector<ComputeShaderStage> create_compute_stages(const glm::ivec2& image_dim, const bool use_v2);
  std::vector<ComputeShaderStage> create_compute_stages_v1(const glm::ivec2& image_dim);
  std::vector<ComputeShaderStage> create_compute_stages_v2(const glm::ivec2& image_dim);
};

}

# endif