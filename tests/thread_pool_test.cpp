#include "threadpool/thread_pool.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {

// 测试辅助函数
void AssertEqual(int expected, int actual, const std::string& message) {
  if (expected != actual) {
    std::cerr << "FAILED: " << message << " - Expected " << expected
              << ", got " << actual << std::endl;
    std::exit(1);
  }
}

void AssertTrue(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << std::endl;
    std::exit(1);
  }
}

// 测试用例
void TestBasicSubmit() {
  std::cout << "Testing BasicSubmit..." << std::endl;
  threadpool::ThreadPool pool(2);
  
  auto future = pool.Submit([]() { return 42; });
  AssertEqual(42, future.get(), "Basic submit result");
  
  std::cout << "  PASSED" << std::endl;
}

void TestMultipleTasks() {
  std::cout << "Testing MultipleTasks..." << std::endl;
  threadpool::ThreadPool pool(4);
  
  std::vector<std::future<int>> futures;
  for (int i = 0; i < 100; ++i) {
    futures.emplace_back(pool.Submit([i]() { return i * 2; }));
  }
  
  for (int i = 0; i < 100; ++i) {
    AssertEqual(i * 2, futures[i].get(), "Multiple tasks result");
  }
  
  std::cout << "  PASSED" << std::endl;
}

void TestExceptionHandling() {
  std::cout << "Testing ExceptionHandling..." << std::endl;
  threadpool::ThreadPool pool(2);
  
  auto future = pool.Submit([]() -> int {
    throw std::runtime_error("Test exception");
    return 0;
  });
  
  bool exception_caught = false;
  try {
    future.get();
  } catch (const std::runtime_error& e) {
    exception_caught = true;
    AssertTrue(std::string(e.what()) == "Test exception", 
               "Exception message");
  }
  
  AssertTrue(exception_caught, "Exception should be caught");
  std::cout << "  PASSED" << std::endl;
}

void TestWaitAll() {
  std::cout << "Testing WaitAll..." << std::endl;
  threadpool::ThreadPool pool(2);
  
  std::atomic<int> counter{0};
  
  for (int i = 0; i < 10; ++i) {
    pool.Submit([&counter]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      ++counter;
    });
  }
  
  pool.WaitAll();
  AssertEqual(10, counter.load(), "All tasks completed");
  
  std::cout << "  PASSED" << std::endl;
}

void TestPendingTasks() {
  std::cout << "Testing PendingTasks..." << std::endl;
  threadpool::ThreadPool pool(1);
  
  AssertEqual(0U, pool.PendingTasks(), "Initial pending tasks");
  
  auto future1 = pool.Submit([]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 1;
  });
  
  auto future2 = pool.Submit([]() { return 2; });
  
  // Give time for first task to start
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  AssertEqual(1U, pool.PendingTasks(), "After submitting second task");
  
  future1.get();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  AssertEqual(0U, pool.PendingTasks(), "After all tasks complete");
  
  std::cout << "  PASSED" << std::endl;
}

void TestShutdownAndRestart() {
  std::cout << "Testing ShutdownAndRestart..." << std::endl;
  threadpool::ThreadPool pool(2);
  
  AssertTrue(pool.IsRunning(), "Pool should be running");
  
  pool.Shutdown();
  AssertTrue(!pool.IsRunning(), "Pool should be stopped");
  
  bool exception_thrown = false;
  try {
    pool.Submit([]() { return 42; });
  } catch (const std::runtime_error&) {
    exception_thrown = true;
  }
  AssertTrue(exception_thrown, "Should throw when submitting to stopped pool");
  
  pool.Restart(3);
  AssertTrue(pool.IsRunning(), "Pool should be running again");
  AssertEqual(3U, pool.WorkerCount(), "Worker count after restart");
  
  auto future = pool.Submit([]() { return 100; });
  AssertEqual(100, future.get(), "Task after restart");
  
  std::cout << "  PASSED" << std::endl;
}

void TestPerformance() {
  std::cout << "Testing Performance..." << std::endl;
  constexpr int kTaskCount = 10000;
  const int thread_count = std::thread::hardware_concurrency();
  
  threadpool::ThreadPool pool(thread_count);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  std::vector<std::future<int>> futures;
  for (int i = 0; i < kTaskCount; ++i) {
    futures.emplace_back(pool.Submit([i]() { return i; }));
  }
  
  for (auto& future : futures) {
    future.get();
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start);
  
  std::cout << "  Executed " << kTaskCount << " tasks in " << duration.count()
            << " ms" << std::endl;
  std::cout << "  PASSED" << std::endl;
}

}  // namespace

int main() {
  std::cout << "=== ThreadPool Unit Tests ===" << std::endl;
  std::cout << "Hardware concurrency: " 
            << std::thread::hardware_concurrency() << std::endl;
  std::cout << "================================" << std::endl;
  
  TestBasicSubmit();
  TestMultipleTasks();
  TestExceptionHandling();
  TestWaitAll();
  TestPendingTasks();
  TestShutdownAndRestart();
  TestPerformance();
  
  std::cout << "================================" << std::endl;
  std::cout << "ALL TESTS PASSED!" << std::endl;
  
  return 0;
}