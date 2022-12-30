#ifndef QUAVIS_UTILS_SHADER_LOADER
#define QUAVIS_UTILS_SHADER_LOADER

#include <memory>

#include "../render/anvil.h"
#include "../logger.h"

namespace quavis {
/// Used whenever a shader is needed. One place that can maybe used later to more smartly cache compile results or load SPIV instead
class ShaderLoader: public UseLogger {
 public:
  static std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> create_shader_entry(std::weak_ptr<Anvil::SGPUDevice> device_ptr, const char *source,
                                                                                 const Anvil::ShaderStage &stage);
};
}  // namespace quavis

#endif