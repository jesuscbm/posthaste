#include "threadpool.hpp"
#include <functional>
#include <optional>
#include <thread>

ThreadPool::ThreadPool(std::optional<size_t> n_threads)
{
	if (!n_threads) {
		n_threads = std::thread::hardware_concurrency();
		if (n_threads == 0) 
			n_threads = 1;
	}
	for (size_t i = 0; i < n_threads; i++) {
		threads.emplace_back([this] {
			for (;;) {
			std::optional<std::function<void()>> task;

				task = queue.wait_pop();
				// Here we have released the queue_mutex
				if (task)
					(*task)();
				else
					break;
			}
		});
	}
}

ThreadPool::~ThreadPool()
{
	queue.shutdown();
	for (std::thread &thread : threads) {
		thread.join();
	}
}

void ThreadPool::addTask(std::function<void()> task)
{
	queue.push(task);
}
