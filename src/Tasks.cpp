#include "Tasks.hpp"
#include "conf.hpp"
#include "Arduino.h"

#include <algorithm>

namespace fablab::tasks
{
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

  void Scheduler::execute()
  {
    for (auto &task : tasks)
    {
      task.get().run();
    }

    if (conf::debug::ENABLE_TASK_LOGS && millis() % 256 == 0)
    {
      std::chrono::milliseconds avg_delay = 0ms;
      unsigned long nb_runs = 0;

      std::for_each(tasks.begin(), tasks.end(),
                    [&avg_delay, &nb_runs](auto &task)
                    {
                      avg_delay += task.get().getAverageDelay() * task.get().getRunCounter();
                      nb_runs += task.get().getRunCounter();
                    });
      if (nb_runs > 0)
      {
        avg_delay /= nb_runs;
      }
      Serial.printf("Scheduler::execute complete: %d tasks total, %lu runs, avg delay/run: %llu ms\r\n", tasks.size(), nb_runs, avg_delay.count());
    }
    else
    {
      delay(3);
    }
  }

  Task::Task(std::string id, std::chrono::milliseconds period, std::function<void()> callback, Scheduler &scheduler, bool active) : active(active),
                                                                                                                                    id(id),
                                                                                                                                    period(period),
                                                                                                                                    last_run(std::chrono::system_clock::now()),
                                                                                                                                    next_run(last_run + period),
                                                                                                                                    average_period(0),
                                                                                                                                    callback(callback),
                                                                                                                                    run_counter(0)
  {
    scheduler.addTask(*this);
  }

  void Task::run()
  {
    if (this->isActive() && std::chrono::system_clock::now() >= next_run)
    {
      run_counter++;
      auto last_period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_run);
      average_period = (average_period * (run_counter - 1) + last_period) / run_counter;
      last_run = std::chrono::system_clock::now();
      try
      {
        callback();
      }
      catch (const std::exception &e)
      {
        Serial.printf("EXCEPTION while executing %s : %s\r\n", this->getId().c_str(), e.what());
      }
      next_run = last_run + period;
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

  void Task::restart()
  {
    last_run = std::chrono::system_clock::now();
    next_run = last_run + period;
  }

  void Task::setPeriod(std::chrono::milliseconds new_period)
  {
    period = new_period;
  }

  void Task::setCallback(std::function<void()> new_callback)
  {
    callback = new_callback;
  }

  void Task::setActive(bool new_active)
  {
    active = new_active;
  }

  bool Task::isActive() const
  {
    return active;
  }

  std::chrono::milliseconds Task::getPeriod() const
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

  std::chrono::milliseconds Task::getAverageDelay() const
  {
    if (average_period > period)
    {
      return average_period - period;
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
} // namespace fablab::tasks