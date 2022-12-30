#include "shader_loader.h"

#include <regex>
#include <misc/glsl_to_spirv.h>

#include "../render/anvil.h"

using namespace std;

std::vector<std::string> Split(const std::string& subject) {
  static const std::regex re{ "\\n" };
  std::vector<std::string> container{
    std::sregex_token_iterator(subject.begin(), subject.end(), re, -1),
    std::sregex_token_iterator()
  };
  return container;
}

std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> quavis::ShaderLoader::create_shader_entry(std::weak_ptr<Anvil::SGPUDevice> device_ptr,
                                                                                              const char* source, const Anvil::ShaderStage& stage)
{
  if (source == nullptr) {
    return make_shared<Anvil::ShaderModuleStageEntryPoint>();
  }

  auto shader = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr, Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE, source, stage);

  try {
    shader->bake_spirv_blob();
    if (shader->get_spirv_blob_size() == 0) {
      throw std::runtime_error(shader->get_shader_info_log());
    }
    auto shader_module = Anvil::ShaderModule::create_from_spirv_generator(device_ptr, shader);
    return make_shared<Anvil::ShaderModuleStageEntryPoint>("main", shader_module, stage);
  }
  catch (std::exception &e) {
    logger_->error("Shader compile Error: {}", e.what());
    auto splitSource = Split(std::string(source));
    auto i = 0;
    for (auto &line : splitSource) {
      logger_->info("{0:>4}: {1}", ++i, line);
    }
    throw e;
  }

}
