#include "threadpool/thread_pool.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// 使用Google风格，函数名使用大驼峰
void PrintThreadInfo() {
  std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
}

int Square(int x) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return x * x;
}

int main() {
  std::cout << "=== ThreadPool Basic Example ===" << std::endl;

  // 创建包含4个线程的线程池
  threadpool::ThreadPool pool(4);
  std::cout << "Worker threads: " << pool.WorkerCount() << std::endl;

  // 提交多个任务并获取结果
  std::vector<std::future<int>> results;
  for (int i = 0; i < 10; ++i) {
    results.emplace_back(pool.Submit(Square, i));
  }

  std::cout << "All tasks submitted, pending: " << pool.PendingTasks()
            << std::endl;

  // 获取结果
  for (size_t i = 0; i < results.size(); ++i) {
    std::cout << "Result of task " << i << ": " << results[i].get()
              << std::endl;
  }

  // 使用lambda提交任务
  auto future = pool.Submit([]() -> int {
    std::cout << "Lambda task executing..." << std::endl;
    return 42;
  });

  std::cout << "Lambda task result: " << future.get() << std::endl;

  // 等待所有任务完成
  pool.WaitAll();
  std::cout << "All tasks completed" << std::endl;

  return 0;
}