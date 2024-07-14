#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#ifndef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL 5
#endif

#undef TAG
[[maybe_unused]] static const char *const TAG = "FAB-O-MATIC"; // Required for ESP32 Logging

#include "esp_log.h"

#endif // LOGGING_HPP_