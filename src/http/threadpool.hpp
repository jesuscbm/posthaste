#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

#include <condition_variable>

class ThreadPool {
   private:
	std::queue<std::function<void()>> tasks;
	std::vector<std::thread> threads;

	std::mutex queue_mutex;
	std::condition_variable cv;

	bool stop = false;

   public:
	ThreadPool(std::optional<size_t> n_threads = std::nullopt);
	~ThreadPool();
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool(ThreadPool &&) = delete;
	ThreadPool &operator=(const ThreadPool &) = delete;
	ThreadPool &operator=(ThreadPool &&) = delete;

	void addTask(std::function<void()>);
};
#endif	// !THREADPOOL_HPP
