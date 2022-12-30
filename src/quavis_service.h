#ifndef QUAVIS_SERVICE
#define QUAVIS_SERVICE

#include <future>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <json/json.hpp>

#include "compute/compute_base.h"
#include "logger.h"
#include "render/render.h"

namespace quavis {

/// The main program logic, which parses a JSON file and behaves accordingly. All members throw errors if needed.
class QuavisService : UseLogger {
 public:
  /// Creates a new Service given JSON.
  QuavisService(const std::shared_ptr<nlohmann::json> &config);

  /// Does most of the work, rendering and computing for all observations
  void run();

  /// Saves all output images or wait until all are saved when saving started during run
  void save_images();

  /// Assembles the output JSON; it blocks until all output images are saved.
  std::shared_ptr<nlohmann::json> get_output_json();

  /// saves the output JSON to the file given in input JSON
  void save_output_json();

 private:  // functions
  /// parses JSON and adds all scene objects to the renderer
  void create_objects(nlohmann::json &j_objects);
  /// parses JSON and add observation points to renderer
  void create_observations(nlohmann::json &j_observations);
  /// parses JSON and creates compute stages
  void create_compute_stages(nlohmann::json &j_computes);
  /// saves the image of observation obs_i of compute stage named stage_name
  std::shared_future<std::string> save_image(size_t obs_i, const std::string &stage_name, std::shared_ptr<ImageCPU> &image);
  /// parses JSON and creates materials
  std::shared_ptr<quavis::MaterialBase> create_material(nlohmann::json &j_material);

 private:
  const std::shared_ptr<nlohmann::json> config_;

  std::shared_ptr<Render> render_;
  std::map<std::string, std::shared_ptr<ComputeBase>> compute_stages_;
  std::vector<std::map<std::string, std::shared_ptr<ComputeResult>>> compute_results_;

  ImageCPU::StoreFormat store_format_;
  std::string image_naming_;
  std::string image_type_;
  std::vector<Observation> observations_;
  int render_width_;
  int render_height_;
};
}  // namespace quavis

#endif