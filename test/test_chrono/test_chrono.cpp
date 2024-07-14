#include <chrono>
#include <string>
#include <functional>

#include <Arduino.h>
#define UNITY_INCLUDE_PRINT_FORMATTED
#include <unity.h>
#include "Tasks.hpp"
#include "Logging.hpp"
#include "Espressif.hpp"

[[maybe_unused]] static const char *TAG4 = "test_chrono";

using namespace std::chrono_literals;

namespace fabomatic::tests
{
  using Task = fabomatic::Tasks::Task;
  using Scheduler = fabomatic::Tasks::Scheduler;

  void tearDown(void)
  {
  }

  void setUp(void)
  {
    // set stuff up here
  }

  void test_steady_clock(void)
  {
    static constexpr auto nb_tests = 100;

    auto cpt = 0;
    TEST_ASSERT_TRUE_MESSAGE(std::chrono::steady_clock::is_steady, "Steady clock available");
    auto previous_val = std::chrono::steady_clock::now();
    while (cpt < nb_tests)
    {
      auto val = std::chrono::steady_clock::now();
      auto duration = (val - previous_val);
      auto count = duration.count();
      std::stringstream ss{};
      ss << "Duration = " << duration << ", tse=" << val.time_since_epoch();
      auto log = ss.str().c_str();
      ESP_LOGI(TAG4, "%s", log);
      TEST_ASSERT_GREATER_THAN_MESSAGE(0, count, "Duration");
      ::delay(10);
      previous_val = val;
      cpt++;
    }
  }

} // namespace fabomatic::tests

void setup()
{
  delay(1000);
  esp_log_level_set(TAG4, LOG_LOCAL_LEVEL);
  UNITY_BEGIN();
  RUN_TEST(fabomatic::tests::test_steady_clock);
  UNITY_END(); // stop unit testing
}

void loop()
{
}