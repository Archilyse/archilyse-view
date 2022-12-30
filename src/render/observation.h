#ifndef QUAVIS_RENDER_OBSERVATION
#define QUAVIS_RENDER_OBSERVATION

#include <glm/glm.hpp>

namespace quavis {
/// Info about one observation point. Here to add later maybe more infos than just position
struct Observation {
  glm::vec3 position;
  glm::vec3 view_direction;
  float field_of_view;
  std::vector<float> solar_azimuth;
  std::vector<float> solar_altitude;
  std::vector<float> solar_zenith_luminance;
  // maybe some more infos about this point
};
}  // namespace quavis

#endif
