#include "Tasks.hpp"
#include "Arduino.h"
#include "conf.hpp"

#include "Arduino.h"
#include "ArduinoOTA.h"
#include <algorithm>
#include <sstream>

#include "Logging.hpp"

namespace fabomatic::Tasks
{
  using milliseconds = std::chrono::milliseconds;

  using namespace std::chrono_literals;

  auto Scheduler::addTask(Task *task) -> void
  {
    if (task != nullptr)
      tasks.push_back(task);
  }

  auto Scheduler::removeTask(const Task *task) -> void
  {
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [&task](const auto &t)
                               { return t->getId() == task->getId(); }),
                tasks.end());
  }

  auto Scheduler::updateSchedules() const -> void
  {
    for (const auto &task : tasks)
    {
      task->updateSchedule();
    }
  }

  auto Scheduler::printStats() const -> void
  {
    milliseconds avg_delay = 0ms;
    auto nb_runs = 0;

    for (const auto &task : tasks)
    {
      avg_delay += task->getAvgTardiness() * task->getRunCounter();
      nb_runs += task->getRunCounter();
    }
    if (nb_runs > 0)
    {
      avg_delay /= nb_runs;
    }

    ESP_LOGD(TAG, "Scheduler::execute complete: %d tasks total, %d runs, avg delay/run: %llu ms\r\n", tasks.size(), nb_runs, avg_delay.count());

    for (const auto &task : tasks)
    {
      if (task->isActive())
      {
        if (task->getRunCounter() > 0)
        {
          ESP_LOGD(TAG, "\t Task: %s, %lu runs, avg tardiness/run: %llu ms, period %llu ms, delay %llu ms, average task duration %llu ms\r\n",
                   task->getId().c_str(), task->getRunCounter(),
                   task->getAvgTardiness().count(), task->getPeriod().count(),
                   task->getDelay().count(),
                   task->getTotalRuntime().count() / task->getRunCounter());
        }
        else
        {
          ESP_LOGD(TAG, "\t Task: %s, never ran, period %llu ms, delay %llu ms\r\n",
                   task->getId().c_str(), task->getPeriod().count(),
                   task->getDelay().count());
        }
      }
      else
      {
        ESP_LOGD(TAG, "\t Task: %s, inactive\r\n", task->getId().c_str());
      }
    }
  }

  auto Scheduler::execute() -> void
  {
    std::sort(tasks.begin(), tasks.end(), [](const auto &x, const auto &y)
              { return x->getNextRun() < y->getNextRun(); });

    for (const auto &t : tasks)
    {
      t->run();
    }

    if (conf::debug::ENABLE_TASK_LOGS && millis() % 1024 == 0)
    {
      printStats();
    }
#if (PINS_WOKWI)
    else
    {
      ::delay(5); // Wokwi simulation is sometimes slow and this helps to catch-up
    }
#endif
  }

  auto Scheduler::taskCount() const -> size_t
  {
    return tasks.size();
  }

  auto Scheduler::getTasks() const -> const std::vector<Task *>
  {
    return {tasks};
  }

  /// @brief Creates a new task
  /// @param id task id
  /// @param period task period (if 0, will be run only once)
  /// @param callback callback function to execute
  /// @param scheduler reference to the scheduler
  /// @param active if true, the task will be executed
  /// @param delay initial delay before starting the task
  Task::Task(const std::string &id, milliseconds period,
             std::function<void()> callback,
             Scheduler &scheduler, bool active, milliseconds delay) : active{active}, id{id},
                                                                      period{period}, delay{delay},
                                                                      next_run{last_run},
                                                                      average_tardiness{0ms}, total_runtime{0ms},
                                                                      callback{callback}, run_counter{0}
  {
    last_run = arduinoNow() + period;
    scheduler.addTask(this);
    if constexpr (conf::debug::ENABLE_TASK_LOGS)
    {
      ESP_LOGD(TAG, "Constructor(%s)\r\n", toString().c_str());
    }
  }

  auto Task::toString() const -> const std::string
  {
    std::stringstream ss;
    ss << "Task " << getId() << ", active=" << active
       << ",Period=" << period << ", Delay=" << delay
       << ",Last run=" << last_run.count()
       << ",Next_run=" << next_run.count()
       << ",Avg tardiness=" << average_tardiness
       << ",total_runtime " << total_runtime
       << ",run_counter=" << run_counter
       << ",clock=" << millis();
    return ss.str();
  }

  auto Task::run() -> void
  {
    auto time_to_run = (arduinoNow() - next_run).count() > 0;
    if (isActive() && time_to_run)
    {
      run_counter++;
      auto last_period = arduinoNow() - last_run;
      average_tardiness = (average_tardiness * (run_counter - 1) + last_period) / run_counter;
      last_run = arduinoNow();

      callback();

      total_runtime += std::chrono::duration_cast<milliseconds>(arduinoNow() - last_run);

      if (period > 0ms)
      {
        next_run = last_run + period; // Schedule next run
      }
      else
      {
        next_run = next_run.max(); // Disable the task
      }

      if constexpr (conf::debug::ENABLE_TASK_LOGS)
      {
        ESP_LOGD(TAG, "Completed(%s)\r\n", toString().c_str());
      }
    }
  }

  auto Task::disable() -> void
  {
    active = false;
  }

  auto Task::enable() -> void
  {
    active = true;
    updateSchedule();
  }

  /// @brief recompute the next run time (now + delay)
  auto Task::updateSchedule() -> void
  {
    last_run = arduinoNow() + delay;
    next_run = last_run;
  }

  auto Task::setPeriod(milliseconds new_period) -> void
  {
    period = new_period;
  }

  auto Task::setCallback(std::function<void()> new_callback) -> void
  {
    callback = new_callback;
  }

  auto Task::isActive() const -> bool
  {
    return active;
  }

  auto Task::getPeriod() const -> milliseconds
  {
    return period;
  }

  auto Task::getCallback() const -> std::function<void()>
  {
    return callback;
  }

  auto Task::getId() const -> const std::string
  {
    return id;
  }

  auto Task::getAvgTardiness() const -> milliseconds
  {
    if (average_tardiness > period)
    {
      return average_tardiness - period;
    }
    return 0ms;
  }

  auto Task::getRunCounter() const -> unsigned long
  {
    return run_counter;
  }

  auto Task::getDelay() const -> milliseconds
  {
    return delay;
  }

  auto Task::setDelay(milliseconds new_delay) -> void
  {
    delay = new_delay;
  }

  auto Task::getTotalRuntime() const -> milliseconds
  {
    return total_runtime;
  }

  auto Task::getNextRun() const -> milliseconds
  {
    return next_run;
  }

  /// @brief Wait for a delay, allowing OTA updates
  /// @param duration period to wait
  auto delay(const milliseconds duration) -> void
  {
    ArduinoOTA.handle();

    if (duration < 50ms)
    {
      ::delay(duration.count());
      return;
    }

    const auto &start = arduinoNow();
    do
    {
      ::delay(50);
      ArduinoOTA.handle();
    } while (arduinoNow() - start < duration);
  }
} // namespace fabomatic::Tasks