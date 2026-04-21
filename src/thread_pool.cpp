#include "threadpool/thread_pool.h"

#include <stdexcept>
#include <utility>

namespace threadpool {

ThreadPool::ThreadPool(size_t num_threads)
    : stop_(false), shutdown_complete_(false) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
      num_threads = 2;  // 默认至少2个线程
    }
  }

  workers_.reserve(num_threads);
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back(&ThreadPool::WorkerLoop, this);
  }
}

ThreadPool::~ThreadPool() {
  Shutdown();
}

void ThreadPool::WorkerLoop() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      condition_.wait(lock, [this] {
        return stop_.load() || !tasks_.empty();
      });

      // 如果线程池已停止且任务队列为空，退出线程
      if (stop_.load() && tasks_.empty()) {
        return;
      }

      // 取出一个任务
      task = std::move(tasks_.front());
      tasks_.pop();
    }

    // 执行任务
    task();

    // 如果任务队列为空，通知可能正在等待的线程
    if (tasks_.empty()) {
      all_tasks_completed_condition_.notify_all();
    }
  }
}

void ThreadPool::WaitAll() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  all_tasks_completed_condition_.wait(lock, [this] {
    return tasks_.empty();
  });
}

size_t ThreadPool::PendingTasks() const {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  return tasks_.size();
}

size_t ThreadPool::WorkerCount() const {
  return workers_.size();
}

void ThreadPool::Shutdown() {
  if (shutdown_complete_.load()) {
    return;
  }

  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_.store(true);
  }

  // 唤醒所有工作线程
  condition_.notify_all();

  // 等待所有线程结束
  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  ClearTasks();
  shutdown_complete_.store(true);
}

void ThreadPool::Restart(size_t num_threads) {
  if (!shutdown_complete_.load()) {
    Shutdown();
  }

  stop_.store(false);
  shutdown_complete_.store(false);

  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
      num_threads = 2;
    }
  }

  workers_.clear();
  workers_.reserve(num_threads);
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back(&ThreadPool::WorkerLoop, this);
  }
}

void ThreadPool::ClearTasks() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  while (!tasks_.empty()) {
    tasks_.pop();
  }
}

}  // namespace threadpool