#ifndef TASKS_HPP_
#define TASKS_HPP_

#include <chrono>
#include <functional>
#include "Arduino.h"
#include <list>

/// @brief This namespace contains the classes that implement a cooperative task scheduler
namespace fabomatic::Tasks
{
  using milliseconds = std::chrono::milliseconds;
  using namespace std::chrono_literals;

  inline auto arduinoNow() -> milliseconds
  {
    return milliseconds{::millis()};
  }

  class Scheduler;

  // A task is a function that is executed periodically
  class Task
  {
  public:
    Task() = delete;

    /// @brief Creates a new task
    /// @param id task id, used for logging
    /// @param period period of the calls
    /// @param callback function to callback
    /// @param scheduler scheduler object to be added into
    /// @param active if false, scheduler will ignore the task until start()/restart() are called on the task
    /// @param delay initial delay before considering the task for execution
    Task(const std::string &id, milliseconds period, std::function<void()> callback, Scheduler &scheduler, bool active = true, milliseconds delay = 0ms);
    ~Task() = default;

    Task(const Task &other) = default;
    Task(Task &&other) = default;
    Task &operator=(const Task &other) = default;
    Task &operator=(Task &&other) = default;

    /// @brief Execute the task if active
    auto run() -> void;

    /// @brief  Prevent the task from running again
    auto disable() -> void;

    /// @brief Allows the task to run again
    auto enable() -> void;

    /// @brief recompute the next run time (now + delay) and allows the task to run again
    auto updateSchedule() -> void;

    /// @brief Change the task period
    /// @param new_period new period. Use 0ms for single shot.
    auto setPeriod(milliseconds new_period) -> void;

    /// @brief Change the task initial delay
    /// @param new_delay Initial delay. Use 0s to avoid any initial delay.
    auto setDelay(milliseconds new_delay) -> void;

    /// @brief Change the callback function
    /// @param new_callback function to be called back
    auto setCallback(std::function<void()> new_callback) -> void;

    /// @brief Status of the task
    /// @return True if Scheduler can launch it
    [[nodiscard]] auto isActive() const -> bool;

    /// @brief Current period of the task
    [[nodiscard]] auto getPeriod() const -> milliseconds;

    /// @brief Function to be called when task is run
    /// @return Callback function
    [[nodiscard]] auto getCallback() const -> std::function<void()>;

    /// @brief Get the Task Identifier
    [[nodiscard]] auto getId() const -> const std::string;

    /// @brief Get the initial delay before the task is run at given period
    /// @return Delay in milliseconds
    [[nodiscard]] auto getDelay() const -> milliseconds;

    /// @brief Get the average tardiness, i.e. the average period between scheduled start and actual start of execution.
    [[nodiscard]] auto getAvgTardiness() const -> milliseconds;

    /// @brief Gets the number of times the task has been run.
    [[nodiscard]] auto getRunCounter() const -> unsigned long;

    /// @brief Gets the total execution time of the task. Useful to spot slowest tasks
    [[nodiscard]] auto getTotalRuntime() const -> milliseconds;

    /// @brief When shall the task be run again
    /// @return time_point of the next run or time_point::max() if the task will not run.
    [[nodiscard]] auto getNextRun() const -> milliseconds;

    [[nodiscard]] auto toString() const -> const std::string;

  private:
    bool active;
    const std::string id;
    milliseconds period;
    milliseconds delay;
    milliseconds last_run;
    milliseconds next_run;
    milliseconds average_tardiness;
    milliseconds total_runtime;
    std::function<void()> callback;
    unsigned long run_counter;
  };

  class Scheduler
  {
  public:
    constexpr Scheduler(){};
    auto addTask(Task *task) -> void;
    auto removeTask(const Task *task) -> void;

    /// @brief Execute all tasks that are ready to run
    /// @details Tasks will be ordered by next_run time ascending, then run sequentially
    auto execute() -> void;

    /// @brief Recompute all the next run times for all the tasks
    auto updateSchedules() const -> void;

    /// @brief Gets the number of tasks in the scheduler
    [[nodiscard]] auto taskCount() const -> size_t;

    /// @brief Get a copy vector of task pointers
    [[nodiscard]] auto getTasks() const -> const std::vector<Task *>;

  private:
    std::vector<Task *> tasks;

    auto printStats() const -> void;
  };

  void delay(const milliseconds delay);

} // namespace fabomatic::Tasks
#endif // TASKS_HPP_