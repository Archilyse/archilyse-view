#include "quavis_service.h"

#include <fstream>

#include <glm/gtc/type_ptr.hpp>
#include <json/json.hpp>

// materials
#include "render/materials/material_env_cube.h"

// compute stages
#include "compute/compute_cube_map.h"
#include "compute/volume.h"
#include "compute/area.h"
#include "compute/groups.h"
#include "compute/sun.h"

using namespace quavis;
using namespace std::string_literals;

QuavisService::QuavisService(const std::shared_ptr<nlohmann::json> &config)
  : config_{config}
{
  if (config->count("quavis") != 1) throw std::runtime_error("JSON: no quavis node found");

  auto &json = config->at("quavis");

  std::string name = json["header"]["name"];
  logger_->info("Start working on: {}", name);

  auto loglevel = json["header"].value("loglevel", spdlog::level::level_enum::info);
  logger_->set_level(loglevel);

  auto &j_render = json["rendering"];
  if (!j_render.is_object()) throw std::runtime_error("JSON: rendering is not an object");

  render_width_  = j_render.value("renderWidth", 512u);
  render_height_ = j_render.value("renderHeight", 512u);
  auto deviceNumber  = j_render.value("gpu", 0u);

  logger_->debug("Create renderer: {1}x{2}", render_width_, render_height_);
  render_ = std::make_shared<quavis::Render>(glm::ivec2(render_width_, render_height_), deviceNumber);

  auto &j_objects = json["sceneObjects"];
  create_objects(j_objects);

  auto &j_observations = json["observationPoints"];
  create_observations(j_observations);

  auto &j_computes = json["computeStages"];
  create_compute_stages(j_computes);
}

void QuavisService::create_objects(nlohmann::json &j_objects)
{
  if (!j_objects.is_array()) throw std::runtime_error("JSON: sceneObjects is not an array");

  logger_->info("Reading {} scene objects", j_objects.size());

  for (auto &obj : j_objects) {
    // modelMatrix
    glm::mat4 model_matrix{1};

    if (obj["modelMatrix"].is_array() && obj["modelMatrix"].size() == 16) {
      std::vector<float> m = obj["modelMatrix"];
      assert(m.size() == 16);
      model_matrix = glm::make_mat4(m.data());
    }

    // material
    auto material = create_material(obj["material"]);

    std::shared_ptr<DrawableGeometry> geom;
    std::string type = obj["type"];
    if (type == "indexedArray"s) {
      std::vector<float> positions   = obj["positions"];
      std::vector<float> vertex_data = obj["vertexData"];
      std::vector<uint32_t> indices  = obj["indices"];
      geom                           = std::make_shared<DrawableGeometry>(std::move(positions), std::move(vertex_data), std::move(indices));
    } else if (type == "unitCube"s) {
      geom = DrawableGeometry::create_unit_cube();
    } else {
      logger_->error("JSON: type {} unknown for sceneObjects", type);
      throw std::runtime_error("JSON: sceneObjects failed");
    }

    render_->add_static_scene_object(std::make_shared<SceneObject>(geom, material, model_matrix));
  }
}

void QuavisService::create_observations(nlohmann::json &j_observations)
{
  if (!j_observations.is_object()) throw std::runtime_error("JSON: observationPoints is not an object");

  auto &j_positions = j_observations["positions"];
  if (!j_positions.is_array() && j_positions.size() % 3 == 0) throw std::runtime_error("JSON: observationPoints.positions is not a valid array");

  auto &j_fovs = j_observations["fieldOfViews"];
  if (!j_fovs.is_array() && j_fovs.size() % 3 == 0) throw std::runtime_error("JSON: observationPoints.fieldOfViews is not a valid array");

  auto &j_view_directions = j_observations["viewDirections"];
  if (!j_view_directions.is_array() && j_view_directions.size() % 3 == 0) throw std::runtime_error("JSON: observationPoints.viewDirections is not a valid array");

  auto &j_solar_azimuths = j_observations["solarAzimuths"];
  if (!j_positions.is_array()) throw std::runtime_error("JSON: observationPoints.positions is not a valid array");

  auto &j_solar_altitudes = j_observations["solarAltitudes"];
  if (!j_positions.is_array()) throw std::runtime_error("JSON: observationPoints.positions is not a valid array");

  auto &j_solar_zenith_luminances = j_observations["solarZenithLuminances"];
  if (!j_positions.is_array()) throw std::runtime_error("JSON: observationPoints.positions is not a valid array");

  std::vector<float> positions = j_positions;
  std::vector<float> fovs = j_fovs;
  std::vector<float> view_directions = j_view_directions;
  std::vector<std::vector<float>> solar_azimuths = j_solar_azimuths;
  std::vector<std::vector<float>> solar_altitudes = j_solar_altitudes;
  std::vector<std::vector<float>> solar_zenith_luminances = j_solar_zenith_luminances;

  std::vector<Observation> observations;
  for (size_t i = 0; i < positions.size(); i += 3) {
    observations.push_back(
      Observation{
        {positions[i + 0], positions[i + 1], positions[i + 2]},
        {view_directions[i + 0], view_directions[i + 1], view_directions[i + 2]},
        fovs[i/3],
        solar_azimuths[i/3],
        solar_altitudes[i/3],
        solar_zenith_luminances[i/3]
      });
    
    observations_.push_back(
      Observation{
        {positions[i + 0], positions[i + 1], positions[i + 2]},
        {view_directions[i + 0], view_directions[i + 1], view_directions[i + 2]},
        fovs[i/3],
        solar_azimuths[i/3],
        solar_altitudes[i/3],
        solar_zenith_luminances[i/3]
      });
  }

  render_->add_observations(std::move(observations));
}

void QuavisService::create_compute_stages(nlohmann::json &j_computes)
{
  if (!j_computes.is_array()) throw std::runtime_error("JSON: computeStages is not an array");

  logger_->info("Reading {} compute stages", j_computes.size());

  for (auto &stage : j_computes) {
    std::string name = stage["name"];

    if (name == "" || compute_stages_.find(name) != compute_stages_.end()) {
      logger_->error("JSON: compute stages name {} is invalid", name);
      throw std::runtime_error("JSON: compute stages name is invalid");
    }

    std::string type = stage["type"];
    if (type == "volume"s) {
      compute_stages_[name] = std::make_shared<ComputeVolume>(render_->get_device(), render_->get_render_size());
    } else if (type == "area"s) {
      compute_stages_[name] = std::make_shared<ComputeArea>(render_->get_device(), render_->get_render_size());
    } else if (type == "groups"s) {
      compute_stages_[name] = std::make_shared<ComputeGroups>(render_->get_device(), render_->get_render_size());
    } else if (type == "sun"s) {
      compute_stages_[name] = std::make_shared<ComputeSun>(render_->get_device(), render_->get_render_size(), false);
    } else if (type == "sunv2"s) {
      compute_stages_[name] = std::make_shared<ComputeSun>(render_->get_device(), render_->get_render_size(), true);
    } else if (type == "cubeMap"s) {
      compute_stages_[name] = std::make_shared<ComputeCubeMap>(stage["pretty"].get<bool>());
    } else {
      logger_->error("JSON: type {} unknown for computeStages", type);
      throw std::runtime_error("JSON: sceneObjects failed");
    }
  }
}

std::shared_ptr<quavis::MaterialBase> quavis::QuavisService::create_material(nlohmann::json &j_material)
{
  std::string type = j_material["type"];
  if (type == "vertexData"s) {
    return std::make_shared<MaterialBase>();
  } else if (type == "environmentMap") {
    std::vector<std::shared_ptr<ImageCPU>> face_images;
    auto &j_faces = j_material["cubeFaces"];
    if (!j_faces.is_array() || j_faces.size() != 6) {
      throw std::runtime_error("JSON: environmentMap faces array is not valid");
    }
    for (auto &face_path : j_faces) {
      face_images.push_back(std::make_shared<ImageCPU>(face_path.get<std::string>()));
    }

    auto texture = std::make_shared<CubeImages>(render_->get_device(), face_images);
    return std::make_shared<MaterialEnvCube>(texture);
  } else {
    logger_->warn(
      "JSON: sceneObjects unknown material type {}, falling back to default "
      "material",
      type);
    return std::make_shared<MaterialBase>();
  }
}

std::shared_future<std::string> quavis::QuavisService::save_image(size_t i, const std::string &name, std::shared_ptr<ImageCPU> &image)
{
  if (image_naming_ == "") {
    auto &j_output = (*config_)["quavis"]["output"];
    if (!j_output.is_object()) throw std::runtime_error("JSON: output is not an object");

    image_naming_ = j_output.value("imageNaming", "{0:0>4}_{1}.png");
    image_type_   = j_output.value("imagesType", "png");

    store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_PNG;
    if (image_type_ == "png") {
      store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_PNG;
    } else if (image_type_ == "hdr") {
      store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_HDR;
    } else if (image_type_ == "json") {
      store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_JSON;
    } else if (image_type_ == "cbor") {
      store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_CBOR;
    } else if (image_type_ == "data") {
      store_format_ = ImageCPU::StoreFormat::IMAGE_CPU_STORE_DATA;
    }
  }

  auto filename = fmt::format(image_naming_, i, name, image_type_);
  return image->wrtie_and_release_async(filename, store_format_);
}

void QuavisService::run()
{
  for (size_t i = 0; i < render_->observations_size(); i++) {
    if (i % 50 == 0)
      logger_->info("Observation: {}", i);
    logger_->debug("Rendering");
    auto image = render_->draw(i);
    logger_->debug("Computing");
    std::map<std::string, std::shared_ptr<ComputeResult>> results;
    std::shared_ptr<ComputeResult> result;
    for (const auto &cs : compute_stages_) {
      logger_->debug("Compute stage '{}'", cs.first);
      if (cs.first == "groups") {
        const ComputeGroupsParams par = {
          render_width_,
          render_height_,
          observations_[i].field_of_view,
          observations_[i].view_direction.x,
          observations_[i].view_direction.y,
          observations_[i].view_direction.z
        };
        std::shared_ptr<ComputeBase> a = cs.second;
        std::shared_ptr<ComputeGroups> b = std::dynamic_pointer_cast<ComputeGroups>(a);
        result = results[cs.first] = b->compute(par, image, image);
      }
      else if (cs.first == "sun") {
        result = std::make_shared<ComputeResult>();
        for(size_t j = 0; j < observations_[i].solar_azimuth.size(); j++) {
          const ComputeSunParams par = {
            render_width_,
            render_height_,
            observations_[i].solar_azimuth[j],
            observations_[i].solar_altitude[j],
            observations_[i].solar_zenith_luminance[j]
          };
          std::shared_ptr<ComputeBase> a = cs.second;
          std::shared_ptr<ComputeSun> b = std::dynamic_pointer_cast<ComputeSun>(a);
          result->values.push_back(b->compute(par, image, image)->values[0]);
        }
        results[cs.first] = result;
      }
      else {
        result = results[cs.first] = cs.second->compute(image, image);
      }

      // save images if needed in multithread
      if (result->image != nullptr) {
        save_image(i, cs.first, result->image);
      }
    }

    compute_results_.push_back(std::move(results));
  }
}

void quavis::QuavisService::save_images()
{
  bool first = true;
  for (size_t i = 0; i < compute_results_.size(); i++) {
    auto &results = compute_results_[i];
    for (auto &r : results) {
      if (r.second->image != nullptr) {
        if (first) {
          logger_->info("Wait for saving images");
          first = false;
        }
        auto ft = save_image(i, r.first, r.second->image);
        ft.wait();
      }
    }
  }
}

namespace quavis {
void to_json(nlohmann::json &j, const ComputeResult &r)
{
  if (r.values.size() > 0) {
    j["values"] = nlohmann::json(r.values);
  }

  if (r.image != nullptr) {
    if (r.image->get_last_filename() == "") {
      j["imagePath"] = "image_not_yet_saved";
    } else {
      j["imagePath"] = r.image->get_last_filename();
    }
  }
}
}  // namespace quavis

namespace nlohmann {
template <typename T>
struct adl_serializer<std::shared_ptr<T>> {
  static void to_json(json &j, const std::shared_ptr<T> opt)
  {
    if (opt == nullptr) {
      j = nullptr;
    } else {
      j = *opt;
    }
  }

  static void from_json(const json &j, std::shared_ptr<T> &opt)
  {
    if (j.is_null()) {
      opt = nullptr;
    } else {
      opt = std::make_shared<T>(j.get<T>());
    }
  }
};
}  // namespace nlohmann

std::shared_ptr<nlohmann::json> QuavisService::get_output_json()
{
  save_images();

  auto result_ptr = std::make_shared<nlohmann::json>();
  auto &result    = *result_ptr;

  result["header"] = config_->at("quavis")["header"];

  result["results"] = compute_results_;

  return result_ptr;
}

void quavis::QuavisService::save_output_json()
{
  auto &j_output = (*config_)["quavis"]["output"];
  if (!j_output.is_object()) throw std::runtime_error("JSON: output is not an object");

  const auto jsonFile = j_output["filename"].get<std::string>();

  if(jsonFile.substr( jsonFile.length() - 4 ) == "msgp") {
    std::ofstream out_file(jsonFile, std::ios::out | std::ios::binary);
    const auto output_json = get_output_json();
    std::vector<std::uint8_t> output_bson = nlohmann::json::to_msgpack(*output_json);
    out_file.write((char*)&output_bson[0], output_bson.size() * sizeof(std::uint8_t));
    out_file.close();
  }
  else {
    std::ofstream out_file(jsonFile);
    const auto output_json = get_output_json();
    out_file << *output_json;
  }

}
