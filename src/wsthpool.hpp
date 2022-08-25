//   Copyright 2021-2022 wxserver - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef WXSERVER_WSTHPOOL_HPP
#define WXSERVER_WSTHPOOL_HPP
#include "wslogger.hpp"

#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <exception>
namespace ws::thpool
{
  class Thpool
  {
  private:
    using Task = std::function<void()>;
    std::vector <std::thread> pool;
    std::queue <Task> tasks;
    std::atomic<bool> run;
    std::mutex th_mutex;
    std::exception_ptr err_ptr;
    std::condition_variable cond;
  public:
    explicit Thpool(std::size_t size) : run(true) { add_thread(size); }
    
    ~Thpool()
    {
      run = false;
      cond.notify_all();
      for (auto &th: pool)
      {
        if (th.joinable()) th.join();
      }
    }
    
    template<typename Func, typename... Args>
    auto add_task(Func &&f, Args &&... args)
    -> std::future<typename std::result_of<Func(Args...)>::type>;
    
    void add_thread(std::size_t num)
    {
      for (std::size_t i = 0; i < num; i++)
      {
        pool.emplace_back(
            [this]
            {
              while (this->run)
              {
                Task task;
                {
                  std::unique_lock <std::mutex> lock(this->th_mutex);
                  this->cond.wait(lock, [this] { return !this->run || !this->tasks.empty(); });
                  if (!this->run && this->tasks.empty()) return;
                  task = std::move(this->tasks.front());
                  this->tasks.pop();
                }
                task();
              }
            }
        );
      }
    }
  };
  
  template<typename Func, typename... Args>
  auto Thpool::add_task(Func &&f, Args &&... args)
  -> std::future<typename std::result_of<Func(Args...)>::type>
  {
    if (!run)
      WS_FATAL("add task on stopped Thpool", -1);
    using ret_type = typename std::result_of<Func(Args...)>::type;
    auto task = std::make_shared < std::packaged_task < ret_type() >>
                                                                   (std::bind(std::forward<Func>(f),
                                                                              std::forward<Args>(args)...));
    std::future <ret_type> ret = task->get_future();
    {
      std::lock_guard <std::mutex> lock(th_mutex);
      tasks.emplace([task] { (*task)(); });
    }
    cond.notify_one();
    return ret;
  }
}
#endif