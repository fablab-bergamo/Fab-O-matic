#include <chrono>
#include <string>
#include <functional>

#include <Arduino.h>
#define UNITY_INCLUDE_PRINT_FORMATTED
#include <unity.h>
#include "Tasks.hpp"
#include "Logging.hpp"
#include "Espressif.hpp"

using namespace std::chrono_literals;

namespace fabomatic::tests
{
  using Task = fabomatic::Tasks::Task;
  using Scheduler = fabomatic::Tasks::Scheduler;

  // Static variables for testing
  constexpr int NB_TASKS = 100;
  bool tasks_status[NB_TASKS]{false};
  Task *tasks[NB_TASKS]{nullptr};
  size_t task_counter{0};
  Scheduler scheduler;
  const auto execute = []()
  { scheduler.execute(); };

  void delete_tasks(void)
  {
    for (auto t : tasks)
    {
      if (t != nullptr)
      {
        scheduler.removeTask(*t); // Because Task adds itself on creation
        delete t;
      }
    }
  }

  void tearDown(void)
  {
    delete_tasks();
  }

  void create_tasks(Scheduler &scheduler, std::chrono::milliseconds period)
  {
    // Clean-up
    task_counter = 0;
    for (auto &t : tasks_status)
    {
      t = false;
    }
    delete_tasks();

    // Creation
    for (size_t i = 0; i < NB_TASKS; ++i)
    {
      auto callback = [i]()
      {
        tasks_status[i] = true;
        task_counter++;
      };
      tasks[i] = new Task("T" + std::to_string(i), period, callback, scheduler, true);
      auto task = *tasks[i];
      TEST_ASSERT_MESSAGE(task.isActive(), "Task is not active");
      TEST_ASSERT_EQUAL_MESSAGE(period.count(), task.getPeriod().count(), "Task period is not 150ms");
    }
  }

  void run_for_duration(std::function<void()> callback, std::chrono::milliseconds duration)
  {
    auto start = fabomatic::Tasks::arduinoNow();
    while (fabomatic::Tasks::arduinoNow() - start < duration)
    {
      callback();
    }
  }

  void setUp(void)
  {
    // set stuff up here
  }

  void test_execute_runs_all_tasks(void)
  {
    create_tasks(scheduler, 150ms);
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, scheduler.taskCount(), "Scheduler does not contain all tasks");

    scheduler.updateSchedules();

    run_for_duration(execute, 140ms);
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, task_counter, "All tasks were not called once in 140 ms");

    delay(50);

    task_counter = 0;
    run_for_duration(execute, 100ms);
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, task_counter, "All tasks were not called once again after 150 ms");
  }

  void test_stop_start_tasks(void)
  {
    create_tasks(scheduler, 150ms);
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, scheduler.taskCount(), "Scheduler does not contain all tasks");

    for (auto *task : tasks)
    {
      task->disable();
    }

    task_counter = 0;
    run_for_duration(execute, 150ms);
    TEST_ASSERT_EQUAL_MESSAGE(0, task_counter, "Stopped tasks has been executed");

    for (auto *task : tasks)
    {
      task->enable();
    }

    task_counter = 0;
    run_for_duration(execute, 150ms);
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, task_counter, "Started tasks have not all been executed");

    task_counter = 0;
    for (auto *task : tasks)
    {
      task->updateSchedule();
    }
    execute();
    TEST_ASSERT_EQUAL_MESSAGE(NB_TASKS, task_counter, "Restarted tasks did not run immediately");
  }

  void test_esp32()
  {
    auto result = fabomatic::esp32::esp_serial_str();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(0, result.length(), "ESP32 serial must be non empty");
    auto mem_free = fabomatic::esp32::getFreeHeap();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(0, mem_free, "ESP32 mem free must be > 0");
    fabomatic::esp32::showHeapStats();
    TEST_ASSERT_TRUE_MESSAGE(fabomatic::esp32::setupWatchdog(30s), "Watchdog setup");
    TEST_ASSERT_TRUE_MESSAGE(fabomatic::esp32::signalWatchdog(), "Watchdog signalling");
    fabomatic::esp32::removeWatchdog();
    auto reset = fabomatic::esp32::esp_reset_reason_str();
    TEST_ASSERT_TRUE_MESSAGE(reset.length() > 0, "Valid reset code");
  }
} // namespace fabomatic::tests

void setup()
{
  delay(1000);
  esp_log_level_set(TAG, LOG_LOCAL_LEVEL);
  UNITY_BEGIN();
  RUN_TEST(fabomatic::tests::test_execute_runs_all_tasks);
  RUN_TEST(fabomatic::tests::test_stop_start_tasks);
  RUN_TEST(fabomatic::tests::test_esp32);
  UNITY_END(); // stop unit testing
}

void loop()
{
}