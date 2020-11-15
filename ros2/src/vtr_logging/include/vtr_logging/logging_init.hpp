// Note: include and only include this header in main.cpp.
#pragma once

#include "vtr_logging/easylogging++.h"
// ** FOLLOWING LINE SHOULD BE USED ONCE AND ONLY ONCE IN WHOLE APPLICATION **
// ** THE BEST PLACE TO PUT THIS LINE IS IN main.cpp RIGHT AFTER INCLUDING
// easylogging++.h **
INITIALIZE_EASYLOGGINGPP

namespace vtr {
namespace logging {

/// Sets up logging in a sane way
void configureLogging(
    const std::string& log_filename  ///< log to this file (unless empty string)
) {
  // Set the config to default
  el::Configurations config;
  config.setToDefault();

  // Set the formats
  // (values are always std::string)
  config.setGlobally(el::ConfigurationType::Format,
                     "%datetime{%H:%m:%s.%g} %level [%fbase:%line] %msg");
  config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");

  // Log to a file if provided
  if (!log_filename.empty()) {
    config.setGlobally(el::ConfigurationType::Filename, log_filename);
    config.setGlobally(el::ConfigurationType::ToFile, "true");
  }

  // Reconfigure loggers and set flags
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
  el::Loggers::addFlag(el::LoggingFlag::LogDetailedCrashReason);
  el::Loggers::reconfigureAllLoggers(config);

  // Reconfigure performance logger
  el::Configurations performanceConfig = config;
  performanceConfig.setGlobally(el::ConfigurationType::ToFile, "true");
  performanceConfig.setGlobally(el::ConfigurationType::ToStandardOutput,
                                "false");
  el::Loggers::reconfigureLogger("performance", performanceConfig);
}

}  // namespace logging
}  // namespace vtr
