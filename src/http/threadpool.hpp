#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <functional>
#include <optional>
#include <thread>
#include <vector>

#include "lockfreequeue.hpp"

class ThreadPool {
   private:
	std::vector<std::thread> threads;
	LockFreeQueue<std::function<void()>> queue;

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
