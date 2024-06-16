#ifndef ESPRESSIF_HPP
#define ESPRESSIF_HPP

#include <chrono>
#include <string_view>

namespace fabomatic::esp32
{
  [[nodiscard]] auto setupWatchdog(std::chrono::milliseconds timeout) -> bool;
  auto signalWatchdog() -> bool;
  auto showHeapStats() -> void;
  auto removeWatchdog() -> void;
  [[nodiscard]] auto esp_serial() -> const std::string_view;
  auto getFreeHeap() -> uint32_t;
  [[noreturn]] auto restart() -> void;
}

#endif // ESPRESSIF_HPP