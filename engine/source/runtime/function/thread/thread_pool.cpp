#include "runtime/function/thread/thread_pool.h"

#include "runtime/core/base/macro.h"

#include <algorithm>
#include <chrono>

namespace MoYu
{
	ThreadPool::ThreadPool(size_t num_threads) :
		stop_flag(false),
		busy_threads(0)
	{
		workers.reserve(num_threads);
		for (size_t i = 0; i < num_threads; ++i)
		{
			workers.emplace_back([this]() { worker_thread(); });
		}
	}

	ThreadPool::~ThreadPool()
	{
		stop();
	}

	void ThreadPool::worker_thread()
	{
		while (!stop_flag.load())
		{
			std::function<void()> task;

			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				condition.wait(lock, [this]() { return stop_flag.load() || !tasks.empty(); });

				if (stop_flag.load() && tasks.empty())
					return;

				task = std::move(tasks.front());
				tasks.pop();
			}

			busy_threads.fetch_add(1);
			task();
			busy_threads.fetch_sub(1);
		}
	}

	void ThreadPool::wait()
	{
		// Wait until all tasks are finished
		while (true)
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			if (tasks.empty() && (busy_threads.load() == 0))
				break;

			lock.unlock();
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	void ThreadPool::stop()
	{
		stop_flag.store(true);
		condition.notify_all();

		for (std::thread& worker : workers)
		{
			if (worker.joinable())
				worker.join();
		}
	}
}