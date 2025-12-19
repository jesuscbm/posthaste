#include "threadpool.hpp"
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

ThreadPool::ThreadPool(std::optional<size_t> n_threads)
{
	if (!n_threads) {
		n_threads = std::thread::hardware_concurrency();
	}
	for (size_t i = 0; i < n_threads; i++) {
		threads.emplace_back([this] {
			for (;;) {
				std::function<void()> task;

				{
					// This waits until the mutex is up (RAII). Which is why we have this context
					std::unique_lock lock(queue_mutex);

					// This releases the mutex again, and waits until notification and predicate,
					// then waits for the mutex again
					cv.wait(lock, [this] { return !tasks.empty() || stop; });

					if (stop && tasks.empty())
						return;

					task = std::move(tasks.front());
					tasks.pop();
				}
				// Here we have released the queue_mutex
				task();
			}
		});
	}
}

ThreadPool::~ThreadPool()
{
	{
		std::unique_lock lock(queue_mutex);
		stop = true;
	}
	cv.notify_all();

	for (std::thread &thread : threads) {
		thread.join();
	}
}

void ThreadPool::addTask(std::function<void()> task)
{
	{
		std::unique_lock lock(queue_mutex);
		tasks.push(task);
	}
	cv.notify_one();
}
