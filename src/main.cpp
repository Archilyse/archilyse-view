#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
namespace fs = std::experimental::filesystem;

#include <json/json.hpp>

#include "compute/volume.h"
#include "logger.h"
#include "quavis_service.h"
#include "render/render.h"

int main(int argc, char* argv[])
{
  auto console = quavis::UseLogger::create_logger();

  try {
    if (argc != 2) {
      throw std::runtime_error("Command Line Arguments invalid: need exactly one argument");
    }

    auto file = std::string(argv[1]);
    console->info("Load input JSON from {}", file);
    auto input_json = std::make_shared<nlohmann::json>();
    if(file.substr( file.length() - 4 ) == "msgp") {
      std::ifstream config_file(file);
      std::vector<uint8_t> contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
      *input_json = nlohmann::json::from_msgpack(contents);
    }
    else {
      std::ifstream config_file(file);
      config_file >> *input_json;
    }

    // change working directory to json file
    fs::path configFile(file);
    auto projectPath = configFile.parent_path();
    console->debug("Set working directory to {}", projectPath.string());
    fs::current_path(projectPath);

    auto service = std::make_shared<quavis::QuavisService>(input_json);
    service->run();
    std::ostringstream s;
    s << std::setw(1) << *service->get_output_json();
    console->trace("Output {}", s.str());
    service->save_output_json();
    console->info("Done");
  } catch (const std::exception& e) {
    console->error("Program Termination: {} ", e.what());
    return 1;
  } catch (...) {
    console->error("Program Termination: Unknown Error");
    return 1;
  }

  return 0;
}
