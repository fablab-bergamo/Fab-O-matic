#ifndef TASKS_H_
#define TASKS_H_

#include <chrono>
#include <functional>

namespace fablabbg::Tasks
{
  using namespace std::chrono;

  class Scheduler;

  class Task
  {
  public:
    Task() = delete;
    Task(std::string id, milliseconds period, std::function<void()> callback, Scheduler &scheduler, bool active);
    ~Task() = default;

    Task(const Task &other) = delete;
    Task(Task &&other) = delete;
    Task &operator=(const Task &other) = delete;
    Task &operator=(Task &&other) = delete;

    void run();
    void stop();
    void start();
    void restart();
    void setPeriod(milliseconds new_period);
    void setCallback(std::function<void()> new_callback);
    void setActive(bool new_active);
    bool isActive() const;
    milliseconds getPeriod() const;
    std::function<void()> getCallback() const;
    std::string getId() const;
    milliseconds getAverageDelay() const;
    unsigned long getRunCounter() const;

  private:
    bool active;
    const std::string id;
    milliseconds period;
    time_point<system_clock> last_run;
    time_point<system_clock> next_run;
    milliseconds average_period;
    std::function<void()> callback;
    unsigned long run_counter;
  };

  class Scheduler
  {
  public:
    Scheduler();
    ~Scheduler() = default;

    void addTask(Task &task);
    void removeTask(Task &task);
    void execute() const;

  private:
    std::vector<std::reference_wrapper<Task>> tasks; // Vector containing references to the tasks, not the tasks themselves
  };
}
#endif