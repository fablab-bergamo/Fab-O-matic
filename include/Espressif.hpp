#ifndef ESPRESSIF_HPP
#define ESPRESSIF_HPP

#include <chrono>
#include <string_view>

/// @brief Contains all espressif32 features not masked by Arduino IDE
namespace fabomatic::esp32
{
  [[nodiscard]] auto setupWatchdog(std::chrono::milliseconds timeout) -> bool;
  auto signalWatchdog() -> bool;
  auto showHeapStats() -> void;
  auto removeWatchdog() -> void;
  [[nodiscard]] auto esp_serial_str() -> const std::string_view;
  auto getFreeHeap() -> uint32_t;
  [[nodiscard]] auto esp_reset_reason_str() -> const std::string_view;
  [[noreturn]] auto restart() -> void;
} // namespace fabomatic::esp32

#endif // ESPRESSIF_HPP