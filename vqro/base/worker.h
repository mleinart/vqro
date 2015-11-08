#ifndef VQRO_WORKER_H
#define VQRO_WORKER_H

#include <deque>
#include <exception>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include "vqro/base/base.h"


DECLARE_int32(worker_task_queue_limit);


namespace vqro {


class WorkerThreadTooBusy: public Error {
 public:
  WorkerThreadTooBusy() { Error("WorkerThreadTooBusy"); }
  WorkerThreadTooBusy(string msg) : Error(msg) {}
  WorkerThreadTooBusy(char* msg) : Error(msg) {}
  virtual ~WorkerThreadTooBusy() {}
};


class WorkerThread {
 public:
  WorkerThread() = default;

  // Disable copying
  WorkerThread(const WorkerThread& other) = delete;

  // Disable assignment
  WorkerThread& operator=(const WorkerThread& other) = delete;

  void Start() {
    my_thread = std::move(
        std::thread([this] { this->DoTasks(); })
    );
    my_thread.detach();
  }

  std::future<void> Do(VoidFunc func) {
    std::future<void> done;
    {
      std::lock_guard<std::mutex> guard(tasks_mutex);
      if (static_cast<int>(tasks.size()) >= FLAGS_worker_task_queue_limit)
        throw WorkerThreadTooBusy("WorkerThreadTooBusy:" + GetThreadId());
      tasks.emplace_back(std::packaged_task<void()>(func));
      done = std::move(tasks.back().get_future());
    }
    tasks_available.notify_one();
    return done;
  }

  size_t TasksQueued() {
    return tasks.size();
  }

  std::condition_variable* Stop() {
    keep_processing = false;
    Do([] {});  // Wake up the DoTasks loop if its waiting on tasks
    return &death;
  }

 private:
  std::thread my_thread;
  bool alive = false;
  bool keep_processing = true;
  std::condition_variable death;

  std::deque<std::packaged_task<void()>> tasks;
  std::mutex tasks_mutex;
  std::condition_variable tasks_available;
  std::mutex tasks_available_mutex;

  void DoTasks() {
    std::unique_lock<std::mutex> wait_lock(tasks_available_mutex);
    std::packaged_task<void()> task;

    alive = true;
    while (keep_processing) {
      // Wait for work to show up
      tasks_available.wait(wait_lock, [&] { return !tasks.empty(); });

      // Safely claim work from the queue
      {
        std::lock_guard<std::mutex> guard(tasks_mutex);
        task = std::move(tasks.front());
        tasks.pop_front();
      }

      // Do the work
      try {
        task();
      } catch (std::exception& e) {
        LOG(ERROR) << "Task exception: " << e.what();
      }

    }
    alive = false;
    death.notify_all();
  }
};


} // namespace vqro


#endif // VQRO_WORKER_H
