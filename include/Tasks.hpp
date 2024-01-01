#ifndef TASKS_H_
#define TASKS_H_

#include <chrono>
#include <functional>

namespace fablabbg::Tasks
{
  using namespace std::chrono;
  using namespace std::chrono_literals;

  class Scheduler;

  class Task
  {
  public:
    Task() = delete;
    Task(const std::string &id, milliseconds period, std::function<void()> callback, Scheduler &scheduler, bool active = true, milliseconds delay = 0ms);
    ~Task() = default;

    Task(const Task &other) = default;
    Task(Task &&other) = default;
    Task &operator=(const Task &other) = default;
    Task &operator=(Task &&other) = default;

    void run();                              // Execute the task if active
    void stop();                             // Prevent the task from running again
    void start();                            // Allows the task to run again
    void restart();                          // Will force the task to run again after delay+period has elapsed
    void setPeriod(milliseconds new_period); // Task period. Set to 0 to run the task only once
    void setDelay(milliseconds new_delay);   // Initial delay before starting the task
    void setCallback(std::function<void()> new_callback);

    bool isActive() const;
    milliseconds getPeriod() const;
    std::function<void()> getCallback() const;
    std::string getId() const;
    milliseconds getDelay() const;
    milliseconds getAvgTardiness() const;
    unsigned long getRunCounter() const;
    milliseconds getTotalRuntime() const;

  private:
    bool active;
    const std::string id;
    milliseconds period;
    milliseconds delay;
    time_point<system_clock> last_run;
    time_point<system_clock> next_run;
    milliseconds average_tardiness;
    milliseconds total_runtime;
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
    void restart() const;
    size_t taskCount() const;

  private:
    std::vector<std::reference_wrapper<Task>> tasks; // Vector containing references to the tasks, not the tasks themselves

    void printStats() const;
  };
}
#endif