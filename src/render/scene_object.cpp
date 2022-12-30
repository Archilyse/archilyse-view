
#include "scene_object.h"

#include <glm/gtc/matrix_transform.hpp>

quavis::SceneObject::SceneObject(std::shared_ptr<DrawableGeometry> geometry, std::shared_ptr<MaterialBase> material, const glm::mat4 &mx)
  : material_{material}
  , geometry_{geometry}
  , shader_data_{mx}
{
}

void quavis::SceneObject::prepare_for_draw(std::weak_ptr<Anvil::SGPUDevice> device_pt)
{
  geometry_->prepare_for_draw(device_pt);
  material_->prepare_for_draw(device_pt);
}

void quavis::SceneObject::draw(std::weak_ptr<Anvil::SGPUDevice> device_ptr, std::shared_ptr<Anvil::PrimaryCommandBuffer> command_buffer)
{
  material_->use(device_ptr, command_buffer);
  geometry_->draw(device_ptr, command_buffer);
}

std::shared_ptr<quavis::MaterialBase> quavis::SceneObject::get_material() const
{
  return material_;
}

std::shared_ptr<quavis::DrawableGeometry> quavis::SceneObject::get_geometry() const
{
  return geometry_;
}

const quavis::SceneObject::ObjectShaderData &quavis::SceneObject::get_shader_data() const
{
  return shader_data_;
}
