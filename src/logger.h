#ifndef QUAVIS_LOGGER
#define QUAVIS_LOGGER

#include <memory>

#ifndef _WIN32
#define SPDLOG_ENABLE_SYSLOG
#endif
#include "spdlog/spdlog.h"

namespace quavis {
/// This class makes logger_ as a member available if you derive from it
class UseLogger {
 public:
   static std::shared_ptr<spdlog::logger> create_logger()
  {
    spdlog::set_async_mode(8192);
    std::vector<spdlog::sink_ptr> sinks;

#ifdef _WIN32
    sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
#else
    sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::syslog_sink>("quavis"));
#endif

    auto logger = std::make_shared<spdlog::logger>("quavis", std::begin(sinks), std::end(sinks));
    spdlog::register_logger(logger);

    logger_      = spdlog::get("quavis");
    return logger_;
  }

 protected:
  UseLogger()
  {
    if (!logger_) {
      create_logger();
    }
  }

  static std::shared_ptr<spdlog::logger> logger_;  ///< reference to the global logger
};

}  // namespace quavis

#endif