#include "config.h"

#include <peel/GLib/GLib.h>
#include <peel/GLib/functions.h>
#include <source_location>

#define debug(format, args...)                                                                     \
  peel::GLib::log (                                                                                \
    APP_ID, peel::GLib::LogLevelFlags::LEVEL_DEBUG, "%s -> " format,                               \
    std::source_location::current ().function_name (), args                                        \
  )

#define warning(format, args...)                                                                   \
  peel::GLib::log (                                                                                \
    APP_ID, peel::GLib::LogLevelFlags::LEVEL_WARNING, "%s -> " format,                             \
    std::source_location::current ().function_name (), args                                        \
  )

#define critical(format, args...)                                                                  \
  peel::GLib::log (                                                                                \
    APP_ID, peel::GLib::LogLevelFlags::LEVEL_CRITICAL, "%s -> " format,                            \
    std::source_location::current ().function_name (), args                                        \
  )
