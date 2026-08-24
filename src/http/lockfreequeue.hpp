#ifndef LOCKFREEQUEUE_HPP
#define LOCKFREEQUEUE_HPP

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <condition_variable>

template<typename T, unsigned int Size = 1024>
class LockFreeQueue {
	static_assert(Size > 0 && (Size & (Size - 1)) == 0, "Size must be a power of 2");
	private: 
		static constexpr unsigned int mask = Size-1;

		struct Cell {
			T item;
			std::atomic_uint32_t sequence;
		};

		std::array<Cell, Size> buffer;
		alignas(64) std::atomic_uint32_t top;
		alignas(64) std::atomic_uint32_t bottom;

		std::mutex wait_mutex;
		std::condition_variable wait_cv;

		std::atomic_bool stop = false;

	public:
		LockFreeQueue();
		~LockFreeQueue();

		void operator= (LockFreeQueue &) = delete;
		LockFreeQueue (LockFreeQueue &) = delete;
		void operator= (LockFreeQueue &&) = delete;
		LockFreeQueue (LockFreeQueue &&) = delete;

		void shutdown();
		T pop();
		std::optional<T> wait_pop();
		void push (T);
};

template<typename T, unsigned int Size>
LockFreeQueue<T, Size>::LockFreeQueue() : 
    top(0), bottom(0) 
{
    for (unsigned int i = 0; i < Size; ++i) {
        buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
}

template<typename T, unsigned int Size>
LockFreeQueue<T, Size>::~LockFreeQueue() {
	stop.store(true, std::memory_order_release);
	wait_cv.notify_all();
}

template<typename T, unsigned int Size>
void LockFreeQueue<T, Size>::shutdown() {
    stop.store(true, std::memory_order_release);
    wait_cv.notify_all();
}


template<typename T, unsigned int Size>
T LockFreeQueue<T, Size>::pop() {
    while (true) {
        unsigned int local_top = top.load(std::memory_order_relaxed);
        unsigned int index = local_top & mask;
        unsigned int local_sequence = buffer[index].sequence.load(std::memory_order_acquire);

        if (local_sequence == local_top + 1) {
            if (top.compare_exchange_weak(local_top, local_top + 1, std::memory_order_relaxed)) {
                T ret = std::move(buffer[index].item);
                buffer[index].sequence.store(local_top + mask + 1, std::memory_order_release);
                return ret;
            }
        }
    }
}

template<typename T, unsigned int Size>
void LockFreeQueue<T, Size>::push (T item) {
	while (true) {
		unsigned int local_bottom = bottom.load(std::memory_order_relaxed);
        unsigned int index = local_bottom & mask;
        unsigned int local_sequence = buffer[index].sequence.load(std::memory_order_acquire);

        if (local_sequence == local_bottom) {
            if (bottom.compare_exchange_weak(local_bottom, local_bottom + 1, std::memory_order_relaxed)) {
                buffer[index].item = std::move(item);
                buffer[index].sequence.store(local_bottom + 1, std::memory_order_release);
				wait_cv.notify_one();
                return;
            }
        }
	}
}

template<typename T, unsigned int Size>
std::optional<T> LockFreeQueue<T, Size>::wait_pop() {
    while (true) {
        unsigned int local_top = top.load(std::memory_order_relaxed);
        unsigned int index = local_top & mask;
        unsigned int local_sequence = buffer[index].sequence.load(std::memory_order_acquire);

        int diff = static_cast<int>(local_sequence - (local_top + 1));

        if (diff == 0) {
            if (top.compare_exchange_weak(local_top, local_top + 1, std::memory_order_relaxed)) {
                T ret = std::move(buffer[index].item);
                buffer[index].sequence.store(local_top + mask + 1, std::memory_order_release);
                return ret;
            }
        } 
        else if (diff < 0) {
            // Empty queue, we sleep
            std::unique_lock<std::mutex> lock(wait_mutex);
            
            wait_cv.wait(lock, [this, local_top, index] {
                unsigned int seq = buffer[index].sequence.load(std::memory_order_acquire);
				bool local_stop = stop.load(std::memory_order_acquire);
                return local_stop || static_cast<int>(seq - (local_top + 1)) >= 0;
            });

			if (stop.load(std::memory_order_acquire))
				return std::nullopt;
        }
        else {
			// Another consumer beat us.
        }
    }
}


#endif
