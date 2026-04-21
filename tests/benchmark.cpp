#include "threadpool/thread_pool.h"

#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace {

// 计算密集型任务
int ComputeIntensive(int n) {
  int result = 0;
  for (int i = 0; i < n; ++i) {
    result += i * i;
  }
  return result;
}

// I/O密集型任务
void IoIntensive(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 基准测试函数
template <typename Func>
double Benchmark(Func&& func, const std::string& name) {
  auto start = std::chrono::high_resolution_clock::now();
  func();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start);
  std::cout << "  " << name << ": " << duration.count() << " ms" << std::endl;
  return static_cast<double>(duration.count());
}

void RunBenchmark(int thread_count, int task_count) {
  std::cout << "Threads: " << thread_count << std::endl;
  
  // 计算密集型任务基准测试
  Benchmark([&]() {
    threadpool::ThreadPool pool(thread_count);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < task_count; ++i) {
      futures.emplace_back(pool.Submit(ComputeIntensive, 10000));
    }
    for (auto& future : futures) {
      future.get();
    }
  }, "Compute intensive");
  
  // I/O密集型任务基准测试
  Benchmark([&]() {
    threadpool::ThreadPool pool(thread_count);
    std::vector<std::future<void>> futures;
    for (int i = 0; i < task_count; ++i) {
      futures.emplace_back(pool.Submit(IoIntensive, 1));
    }
    for (auto& future : futures) {
      future.get();
    }
  }, "I/O intensive");
  
  std::cout << std::endl;
}

}  // namespace

int main() {
  std::cout << "=== ThreadPool Performance Benchmark ===" << std::endl;
  std::cout << "Hardware concurrency: " 
            << std::thread::hardware_concurrency() << std::endl;
  std::cout << std::endl;
  
  constexpr int kTaskCount = 1000;
  std::vector<int> thread_counts = {1, 2, 4, 8, 16};
  
  for (int thread_count : thread_counts) {
    // 不要创建超过硬件并发数2倍的线程
    if (thread_count > static_cast<int>(
            std::thread::hardware_concurrency()) * 2) {
      continue;
    }
    RunBenchmark(thread_count, kTaskCount);
  }
  
  return 0;
}