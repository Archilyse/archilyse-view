#ifndef QUAVIS_COMPUTE_CUBE_MAP
#define QUAVIS_COMPUTE_CUBE_MAP

#include "./compute_base.h"

namespace quavis {

/// no real computation just retrieve the rendered color image
class ComputeCubeMap : public ComputeBase {
 public:
  ComputeCubeMap(bool pretty = false)
    : pretty_{pretty}
  {
  }

  virtual std::shared_ptr<ComputeResult> compute(std::shared_ptr<CubeImages> render_result, std::shared_ptr<CubeImages> depth) override
  {
    auto image = render_result->retrieve_images(pretty_);

    auto result   = std::make_shared<ComputeResult>();
    result->image = std::move(image);

    return result;
  }

 private:
  bool pretty_;
};
}  // namespace quavis

#endif