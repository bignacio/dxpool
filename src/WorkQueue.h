#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>

#include "TypePolicies.h"

namespace dxpool {

/**
 * @brief General thread safe work queue, it holds tasks to be executed and notifies waiting threads
 * whenever a new task is added to the queue.
 *
 */
class WorkQueue final {
  public:
    /**
     * @brief Type of task to be added to the queue
     *
     */
    using WorkerTask = std::function<void()>;
  private:
    std::queue<WorkerTask> tasks{};
    std::condition_variable tasksCondVar;
    std::mutex tasksMutex;
  public:
    WorkQueue() = default;

    /**
     * @brief Adds a new task to the queue
     *
     * @param task task to be queued up
     */
    auto Add(WorkerTask&& task) -> void {
        std::lock_guard<std::mutex> guard(this->tasksMutex);
        this->tasks.push(task);
        this->tasksCondVar.notify_one();
    }

    /**
     * @brief Removes a task from the queue
     *
     * @return WorkerTask task dequeued
     */
    auto Take() -> WorkerTask {
        std::unique_lock<std::mutex> tasksLock(this->tasksMutex);
        this->tasksCondVar.wait(tasksLock, [this] { return !this->tasks.empty();});

        WorkerTask task = this->tasks.front();
        this->tasks.pop();
        return task;
    }

    /**
     * @brief Determines if the queue has any task
     *
     * @return true if the work queue has tasks
     * @return false otherwise
     */
    auto HasWork() -> bool {
        std::lock_guard<std::mutex> guard(this->tasksMutex);
        return !this->tasks.empty();
    }

    FORBID_COPY_MOVE_ASSIGN(WorkQueue);
    ~WorkQueue() = default;
};

} // namespace dxpool

#endif // WORK_QUEUE_H