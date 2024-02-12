#ifndef TASKS_HPP_
#define TASKS_HPP_

#include <chrono>
#include <functional>

/// @brief This namespace contains the classes that implement a cooperative task scheduler
namespace fablabbg::Tasks
{
  using milliseconds = std::chrono::milliseconds;
  using time_point_sc = std::chrono::time_point<std::chrono::system_clock>;
  using namespace std::chrono_literals;

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
    void run();

    /// @brief  Prevent the task from running again
    void stop();

    /// @brief Allows the task to run again
    void start();

    /// @brief recompute the next run time (now + delay) and allows the task to run again
    void restart();

    /// @brief Change the task period
    /// @param new_period new period. Use 0ms for single shot.
    void setPeriod(milliseconds new_period);

    /// @brief Change the task initial delay
    /// @param new_delay Initial delay. Use 0s to avoid any initial delay.
    void setDelay(milliseconds new_delay);

    /// @brief Change the callback function
    /// @param new_callback function to be called back
    void setCallback(std::function<void()> new_callback);

    /// @brief Status of the task
    /// @return True if Scheduler can launch it
    bool isActive() const;

    /// @brief Current period of the task
    [[nodiscard]] milliseconds getPeriod() const;

    [[nodiscard]] std::function<void()> getCallback() const;

    /// @brief Get the Task Identifier
    [[nodiscard]] std::string getId() const;

    [[nodiscard]] milliseconds getDelay() const;

    /// @brief Get the average tardiness, i.e. the average period between scheduled start and actual start of execution.
    [[nodiscard]] milliseconds getAvgTardiness() const;

    /// @brief Gets the number of times the task has been run.
    [[nodiscard]] unsigned long getRunCounter() const;

    /// @brief Gets the total execution time of the task. Useful to spot slowest tasks
    [[nodiscard]] milliseconds getTotalRuntime() const;

    /// @brief When shall the task be run again
    /// @return time_point of the next run or time_point::max() if the task will not run.
    [[nodiscard]] time_point_sc getNextRun() const;

  private:
    bool active;
    const std::string id;
    milliseconds period;
    milliseconds delay;
    time_point_sc last_run;
    time_point_sc next_run;
    milliseconds average_tardiness;
    milliseconds total_runtime;
    std::function<void()> callback;
    unsigned long run_counter;
  };

  class Scheduler
  {
  public:
    Scheduler() noexcept;
    ~Scheduler() = default;

    void addTask(Task &task);
    void removeTask(Task &task);

    /// @brief Execute all tasks that are ready to run
    /// @details Tasks will be ordered by next_run time ascending, then run sequentially
    void execute() const;

    /// @brief Recompute all the next run times for all the tasks
    void restart() const;

    /// @brief Gets the number of tasks in the scheduler
    size_t taskCount() const;

    /// @brief Get a vector of references to the tasks
    const std::vector<std::reference_wrapper<Task>> getTasks() const;

  private:
    std::vector<std::reference_wrapper<Task>> tasks; // Vector containing references to the tasks, not the tasks themselves

    void printStats() const;
  };

  void task_delay(const milliseconds delay);

} // namespace fablabbg::Tasks
#endif // TASKS_HPP_