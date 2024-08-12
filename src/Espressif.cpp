#include <string>
#include <sstream>
#include <array>
#include <iomanip>

#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "esp_mac.h"

#include "Espressif.hpp"
#include "Logging.hpp"

namespace fabomatic::esp32
{
  void showHeapStats()
  {
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    // Get heap summary
    multi_heap_info_t heap_info;
    heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

    // Log heap statistics
    ESP_LOGI("Heap Stats", "Total Free Heap Size: %zu bytes", heap_info.total_free_bytes);
    ESP_LOGI("Heap Stats", "Total Allocated Size: %zu bytes", heap_info.total_allocated_bytes);
    ESP_LOGI("Heap Stats", "Largest Free Block: %zu bytes", heap_info.largest_free_block);
    ESP_LOGI("Heap Stats", "Min Free Heap Size Ever: %zu bytes", heap_info.minimum_free_bytes);
    ESP_LOGI("Heap Stats", "Free Blocks: %zu", heap_info.free_blocks);
    ESP_LOGI("Heap Stats", "Allocated Blocks: %zu", heap_info.allocated_blocks);
  }

  auto setupWatchdog(std::chrono::milliseconds msecs) -> bool
  {
    removeWatchdog();

#if ESP_IDF_VERSION_MAJOR < 5
    auto secs = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(msecs).count());
    auto retVal = esp_task_wdt_init(secs, true);
    ESP_LOGI(TAG, "taskEspWatchdog - initialized %u seconds", secs);
#else
    auto ui_msecs = static_cast<uint32_t>(msecs.count());
    esp_task_wdt_config_t conf{.timeout_ms = ui_msecs, .idle_core_mask = 1, .trigger_panic = true};
    auto retVal = esp_task_wdt_init(&conf);
    ESP_LOGI(TAG, "taskEspWatchdog - initialized %lu milliseconds", ui_msecs);
#endif

    if (retVal != ESP_OK)
    {
      return false;
    }

    retVal = esp_task_wdt_add(NULL); // add current thread to WDT watch
    return (retVal == ESP_OK);
  }

  auto signalWatchdog() -> bool
  {
    return esp_task_wdt_reset() == ESP_OK;
  }

  auto removeWatchdog() -> void
  {
    esp_task_wdt_deinit();
  }

  /// @brief Returns the ESP32 serial number as a string
  [[nodiscard]] auto esp_serial_str() -> const std::string_view
  {
    static std::array<char, 13> result; // +1 for null termination

    if (result[0] == '\0') // Compute only once
    {
      std::stringstream serial{};
      std::array<uint8_t, 8> mac{0};

      esp_efuse_mac_get_default(mac.data());
      for (const auto val : mac)
      {
        serial << std::setfill('0') << std::setw(2) << std::hex << +val;
      }
      auto res = serial.str().substr(0U, 6 * 2);
      std::copy(res.cbegin(), res.cend(), result.begin());
      result[result.size() - 1] = '\0';
    }
    return {result.data()};
  }

  auto getFreeHeap() -> uint32_t
  {
    return esp_get_free_heap_size();
  }

  [[noreturn]] auto restart() -> void
  {
    esp_restart();
  }

  auto esp_reset_reason_str() -> const std::string_view
  {
    switch (esp_reset_reason())
    {
    case ESP_RST_UNKNOWN: //!< Reset reason can not be determined
      return "Unknown";
    case ESP_RST_POWERON: //!< Reset due to power-on event
      return "Power-on reset";
    case ESP_RST_EXT: //!< Reset by external pin (not applicable for ESP32)
      return "External pin reset";
    case ESP_RST_SW: //!< Software reset via esp_restart
      return "Software reset";
    case ESP_RST_PANIC: //!< Software reset due to exception/panic
      return "Panic";
    case ESP_RST_INT_WDT: //!< Reset (software or hardware) due to interrupt watchdog
      return "Interrupt watchdog reset";
    case ESP_RST_TASK_WDT: //!< Reset due to task watchdog
      return "Task watchdog reset";
    case ESP_RST_WDT: //!< Reset due to other watchdogs
      return "Watchdog reset";
    case ESP_RST_DEEPSLEEP: //!< Reset after exiting deep sleep mode
      return "Deepsleep exit";
    case ESP_RST_BROWNOUT: //!< Brownout reset (software or hardware)
      return "Brownout";
    case ESP_RST_SDIO: //!< Reset over SDIO
      return "SDIO reset";
    case ESP_RST_USB: //!< Reset by USB peripheral
      return "USB reset";
    case ESP_RST_JTAG: //!< Reset by JTAG
      return "JTAG reset";
    case ESP_RST_EFUSE: //!< Reset due to efuse error
      return "EFUSE error";
    case ESP_RST_PWR_GLITCH: //!< Reset due to power glitch detected
      return "Power glitch";
    case ESP_RST_CPU_LOCKUP: //!< Reset due to CPU lock up
      return "CPU lock up";
    default:
      return "UNKNOWN";
    }
  }
}