#include "Tasks.hpp"
#include "conf.hpp"
#include "Arduino.h"

#include <algorithm>
#include "Arduino.h"
#include "ArduinoOTA.h"

#include "Logging.hpp"

namespace fablabbg::Tasks
{
  using namespace std::chrono;
  using namespace std::chrono_literals;

  Scheduler::Scheduler() noexcept : tasks()
  {
  }

  void Scheduler::addTask(Task &task)
  {
    tasks.push_back(task);
  }

  void Scheduler::removeTask(Task &task)
  {
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [&task](const auto &t)
                               { return t.get().getId() == task.getId(); }),
                tasks.end());
  }

  void Scheduler::restart() const
  {
    for (const auto &task : tasks)
    {
      task.get().restart();
    }
  }
  void Scheduler::printStats() const
  {
    milliseconds avg_delay = 0ms;
    unsigned long nb_runs = 0;

    for (const auto &task : tasks)
    {
      avg_delay += task.get().getAvgTardiness() * task.get().getRunCounter();
      nb_runs += task.get().getRunCounter();
    }
    if (nb_runs > 0)
    {
      avg_delay /= nb_runs;
    }

    ESP_LOGD(TAG, "Scheduler::execute complete: %d tasks total, %lu runs, avg delay/run: %llu ms\r\n", tasks.size(), nb_runs, avg_delay.count());

    for (const auto &task : tasks)
    {
      if (task.get().isActive())
      {
        if (task.get().getRunCounter() > 0)
        {
          ESP_LOGD(TAG, "\t Task: %s, %lu runs, avg tardiness/run: %llu ms, period %llu ms, delay %llu ms, average task duration %llu ms\r\n",
                   task.get().getId().c_str(), task.get().getRunCounter(),
                   task.get().getAvgTardiness().count(), task.get().getPeriod().count(),
                   task.get().getDelay().count(),
                   task.get().getTotalRuntime().count() / task.get().getRunCounter());
        }
        else
        {
          ESP_LOGD(TAG, "\t Task: %s, never ran, period %llu ms, delay %llu ms\r\n",
                   task.get().getId().c_str(), task.get().getPeriod().count(),
                   task.get().getDelay().count());
        }
      }
      else
      {
        ESP_LOGD(TAG, "\t Task: %s, inactive\r\n", task.get().getId().c_str());
      }
    }
  }

  void Scheduler::execute() const
  {
    // Tasks shall be run in order of expiration (the most expired task shall run first)
    std::vector<decltype(tasks)::const_iterator> iters;
    for (auto it = tasks.begin(); it != tasks.end(); ++it)
    {
      iters.push_back(it); // Vector of iterators
    }

    // Sort the iterators array as we cannot sort directly reference_wrappers
    std::sort(iters.begin(), iters.end(),
              [](const auto &it1, const auto &it2)
              {
                return (it1->get().getNextRun() < it2->get().getNextRun());
              });

    // Now iterate over the sorted iterators to run the tasks
    for (const auto &it : iters)
    {
      it->get().run();
    }

    if (conf::debug::ENABLE_TASK_LOGS && millis() % 1024 == 0)
    {
      printStats();
    }
#if (WOKWI_SIMULATION)
    else
    {
      delay(5); // Wokwi simulation is sometimes slow and this helps to catch-up
    }
#endif
  }

  size_t Scheduler::taskCount() const
  {
    return tasks.size();
  }

  const std::vector<std::reference_wrapper<Task>> Scheduler::getTasks() const
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
             Scheduler &scheduler, bool active, milliseconds delay) : active(active), id(id),
                                                                      period(period), delay(delay),
                                                                      last_run(system_clock::now() + delay),
                                                                      next_run(last_run),
                                                                      average_tardiness(0ms), total_runtime(0ms),
                                                                      callback(callback), run_counter(0)
  {
    scheduler.addTask(std::ref(*this));
  }

  void Task::run()
  {
    if (isActive() && system_clock::now() >= next_run)
    {
      run_counter++;
      auto last_period = duration_cast<milliseconds>(system_clock::now() - last_run);
      average_tardiness = (average_tardiness * (run_counter - 1) + last_period) / run_counter;
      last_run = system_clock::now();

      if (conf::debug::ENABLE_TASK_LOGS)
      {
        ESP_LOGD(TAG, "Task %s\r\n", getId().c_str());
      }

      callback();

      total_runtime += duration_cast<milliseconds>(system_clock::now() - last_run);

      if (period > 0ms)
      {
        next_run = last_run + period; // Schedule next run
      }
      else
      {
        next_run = system_clock::time_point::max(); // Disable the task
      }
    }
  }

  void Task::stop()
  {
    active = false;
  }

  void Task::start()
  {
    active = true;
    restart();
  }

  /// @brief recompute the next run time (now + delay)
  void Task::restart()
  {
    last_run = system_clock::now() + delay;
    next_run = last_run;
  }

  void Task::setPeriod(milliseconds new_period)
  {
    period = new_period;
  }

  void Task::setCallback(std::function<void()> new_callback)
  {
    callback = new_callback;
  }

  bool Task::isActive() const
  {
    return active;
  }

  milliseconds Task::getPeriod() const
  {
    return period;
  }

  std::function<void()> Task::getCallback() const
  {
    return callback;
  }

  std::string Task::getId() const
  {
    return id;
  }

  milliseconds Task::getAvgTardiness() const
  {
    if (average_tardiness > period)
    {
      return average_tardiness - period;
    }
    return 0ms;
  }

  unsigned long Task::getRunCounter() const
  {
    return run_counter;
  }

  milliseconds Task::getDelay() const
  {
    return delay;
  }

  void Task::setDelay(milliseconds new_delay)
  {
    delay = new_delay;
  }

  milliseconds Task::getTotalRuntime() const
  {
    return total_runtime;
  }

  time_point<system_clock> Task::getNextRun() const
  {
    return next_run;
  }

  /// @brief Wait for a delay, allowing OTA updates
  /// @param delay period to wait (should be > 50 ms)
  void task_delay(const milliseconds duration)
  {
    if (duration < 50ms)
    {
      delay(duration.count());
      return;
    }

    const auto start = system_clock::now();
    do
    {
      delay(50);
      ArduinoOTA.handle();
    } while (system_clock::now() - start < duration);
  }
} // namespace fablabbg::Tasks