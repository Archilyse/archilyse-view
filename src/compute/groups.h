#ifndef QUAVIS_COMPUTE_GROUPS
#define QUAVIS_COMPUTE_GROUPS

#include "./compute_base.h"

namespace quavis {
struct ComputeGroupsParams {
  int width;
  int height;
  float field_of_view;
  float view_x;
  float view_y;
  float view_z;
};

class ComputeGroups : public ComputeBaseGPU<ComputeGroupsParams> {
 public:
  ComputeGroups(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const glm::ivec2& image_dim);
  virtual const ComputeGroupsParams get_parameter() override;

 private:
  const glm::ivec2 image_dim_;

  std::vector<ComputeShaderStage> create_compute_stages(const glm::ivec2& image_dim);
};
}  // namespace quavis

#endif