#include <iostream>
#include <thread>
#include <functional>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    // Lock the queue
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);

                        condition.wait(lock, [this] {
                            return stop || !task_queue.empty();
                        });

                        if (stop && task_queue.empty()) {
                            return;
                        }

                        task = std::move(task_queue.front());
                        task_queue.pop();
                    }

                    // Execute the task
                    task();
                }
            });
        }
    }

    // Enqueue a task to the thread pool
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            task_queue.push(std::move(task));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> task_queue;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};
