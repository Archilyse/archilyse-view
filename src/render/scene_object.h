#ifndef QUAVIS_RENDER_SCENE_OBJECT
#define QUAVIS_RENDER_SCENE_OBJECT

#include <memory>

#include <glm/glm.hpp>

#include "./drawable_geometry.h"
#include "./materials/material_base.h"

namespace quavis {
/// The base class of an object that can be rendered. Its made from drawable geometry, one material and one model matrix.
class SceneObject {
 public:
  struct ObjectShaderData {
    glm::mat4 model_matrix;
  };

  /// Creates a Scene object with one drawable geometry, one material and one model matrix.
  SceneObject(std::shared_ptr<DrawableGeometry> geometry, std::shared_ptr<MaterialBase> material, const glm::mat4 &model_matrix = glm::mat4(1));

  /// called only ones before any draw happens. Use it to init GPU data structures (VBO, textures,...)
  void prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt);

  /// draws that scene object
  void draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer);

  std::shared_ptr<MaterialBase> get_material() const;
  std::shared_ptr<DrawableGeometry> get_geometry() const;

  /// returns the ShaderData (model matrix) that should be pushed by the renderer.
  const ObjectShaderData &get_shader_data() const;

 private:
  std::shared_ptr<DrawableGeometry> geometry_;
  std::shared_ptr<MaterialBase> material_;

  ObjectShaderData shader_data_;
};
}  // namespace quavis

#endif
