#include "Tasks.hpp"
#include "conf.hpp"
#include "Arduino.h"

#include <algorithm>

namespace fablabbg::Tasks
{
  using namespace std::chrono;
  using namespace std::chrono_literals;

  Scheduler::Scheduler() : tasks()
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

    Serial.printf("Scheduler::execute complete: %d tasks total, %lu runs, avg delay/run: %llu ms\r\n", tasks.size(), nb_runs, avg_delay.count());

    for (const auto &task : tasks)
    {
      if (task.get().isActive())
      {
        if (task.get().getRunCounter() > 0)
        {
          Serial.printf("\t Task: %s, %lu runs, avg tardiness/run: %llu ms, period %llu ms, delay %llu ms, average task duration %llu ms\r\n",
                        task.get().getId().c_str(), task.get().getRunCounter(),
                        task.get().getAvgTardiness().count(), task.get().getPeriod().count(),
                        task.get().getDelay().count(),
                        task.get().getTotalRuntime().count() / task.get().getRunCounter());
        }
        else
        {
          Serial.printf("\t Task: %s, never ran, period %llu ms, delay %llu ms\r\n",
                        task.get().getId().c_str(), task.get().getPeriod().count(),
                        task.get().getDelay().count());
        }
      }
      else
      {
        Serial.printf("\t Task: %s, inactive\r\n", task.get().getId().c_str());
      }
    }
  }

  void Scheduler::execute() const
  {
    for (const auto &task : tasks)
    {
      task.get().run();
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

  /// @brief Creates a new task
  /// @param id task id
  /// @param period task period (if 0, will be run only once)
  /// @param callback callback function to execute
  /// @param scheduler reference to the scheduler
  /// @param active if true, the task will be executed
  /// @param delay initial delay before starting the task
  Task::Task(std::string id, milliseconds period,
             std::function<void()> callback,
             Scheduler &scheduler, bool active, milliseconds delay) : active(active), id(id),
                                                                      period(period), delay(delay),
                                                                      last_run(system_clock::now() + delay),
                                                                      next_run(last_run),
                                                                      average_tardiness(0ms), total_runtime(0ms),
                                                                      callback(callback), run_counter(0)
  {
    scheduler.addTask(*this);
  }

  void Task::run()
  {
    if (this->isActive() && system_clock::now() >= next_run)
    {
      run_counter++;
      auto last_period = duration_cast<milliseconds>(system_clock::now() - last_run);
      average_tardiness = (average_tardiness * (run_counter - 1) + last_period) / run_counter;
      last_run = system_clock::now();

      if (conf::debug::ENABLE_TASK_LOGS)
      {
        Serial.printf("Task %s\r\n", this->getId().c_str());
      }

      try
      {
        callback();
      }
      catch (const std::exception &e)
      {
        Serial.printf("EXCEPTION while executing %s : %s\r\n", this->getId().c_str(), e.what());
      }

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
    else
    {
      return 0ms;
    }
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
} // namespace fablab::tasks