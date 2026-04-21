#ifndef THREADPOOL_THREAD_POOL_H_
#define THREADPOOL_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace threadpool {

// 线程池类，用于管理和执行异步任务
//
// 使用示例:
//   ThreadPool pool(4);
//   auto future = pool.Submit([](int x) { return x * x; }, 5);
//   int result = future.get();  // result = 25
class ThreadPool {
 public:
  // 构造函数，创建指定数量的工作线程
  // 参数:
  //   num_threads: 工作线程数量，如果为0则使用硬件并发数
  explicit ThreadPool(size_t num_threads = 0);

  // 析构函数，确保所有线程安全退出
  ~ThreadPool();

  // 禁止拷贝构造和赋值操作
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // 允许移动构造和移动赋值
  ThreadPool(ThreadPool&&) = default;
  ThreadPool& operator=(ThreadPool&&) = default;

  // 提交一个任务到线程池
  //
  // 模板参数:
  //   F: 可调用对象类型
  //   Args: 参数类型包
  // 参数:
  //   f: 可调用对象（函数、lambda、函数对象等）
  //   args: 传递给可调用对象的参数
  // 返回:
  //   一个std::future对象，用于获取任务的返回值或异常
  //
  // 示例:
  //   auto future = pool.Submit([](int a, int b) { return a + b; }, 10, 20);
  //   int result = future.get();  // result = 30
  template <typename F, typename... Args>
  auto Submit(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result_t<F, Args...>>;

  // 立即执行任务（不排队），在一个新线程中执行
  //
  // 模板参数:
  //   F: 可调用对象类型
  //   Args: 参数类型包
  // 参数:
  //   f: 可调用对象
  //   args: 传递给可调用对象的参数
  // 返回:
  //   一个std::future对象，用于获取任务的返回值或异常
  template <typename F, typename... Args>
  auto SubmitImmediate(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result_t<F, Args...>>;

  // 等待所有已提交的任务完成
  void WaitAll();

  // 获取当前待处理的任务数量
  size_t PendingTasks() const;

  // 获取工作线程数量
  size_t WorkerCount() const;

  // 获取线程池是否正在运行
  bool IsRunning() const { return !stop_; }

  // 关闭线程池，不再接受新任务
  void Shutdown();

  // 重启线程池
  // 参数:
  //   num_threads: 新的工作线程数量，如果为0则使用硬件并发数
  void Restart(size_t num_threads = 0);

 private:
  // 工作线程的主循环函数
  void WorkerLoop();

  // 清空任务队列
  void ClearTasks();

  // 工作线程集合
  std::vector<std::thread> workers_;

  // 任务队列
  std::queue<std::function<void()>> tasks_;

  // 保护任务队列的互斥锁
  mutable std::mutex queue_mutex_;

  // 用于唤醒工作线程的条件变量
  std::condition_variable condition_;

  // 用于等待所有任务完成的条件变量
  std::condition_variable all_tasks_completed_condition_;

  // 线程池停止标志
  std::atomic<bool> stop_;

  // 关闭完成标志
  std::atomic<bool> shutdown_complete_;
};

// 模板方法实现
template <typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result_t<F, Args...>> {
  using return_type = typename std::invoke_result_t<F, Args...>;

  // 创建包装任务
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> result = task->get_future();

  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (stop_) {
      throw std::runtime_error("ThreadPool is stopped, cannot submit new task");
    }

    // 将任务添加到队列
    tasks_.emplace([task]() { (*task)(); });
  }

  // 唤醒一个工作线程
  condition_.notify_one();
  return result;
}

template <typename F, typename... Args>
auto ThreadPool::SubmitImmediate(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result_t<F, Args...>> {
  using return_type = typename std::invoke_result_t<F, Args...>;

  // 创建promise和future
  auto promise = std::make_shared<std::promise<return_type>>();
  std::future<return_type> result = promise->get_future();

  // 在新线程中执行任务
  std::thread([promise, f = std::forward<F>(f), args...]() mutable {
    if constexpr (std::is_void_v<return_type>) {
      f(args...);
      promise->set_value();
    } else {
      promise->set_value(f(args...));
    }
  }).detach();

  return result;
}

}  // namespace threadpool

#endif  // THREADPOOL_THREAD_POOL_H_