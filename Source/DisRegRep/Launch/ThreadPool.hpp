#pragma once

#include <functional>
#include <memory>
#include <queue>

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <exception>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace DisRegRep {

/**
 * @brief A pool of reusable threads.
*/
class ThreadPool {
public:

	/**
	 * @brief Additional information regarding the thread used to execute the job.
	*/
	struct ThreadInfo {

		size_t ThreadIndex;/**< Index of the thread within the thread pool. */

	};

private:

	std::queue<std::move_only_function<void(const ThreadInfo&)>> Job;

	std::mutex Mutex;
	std::condition_variable Signal;

	std::unique_ptr<std::jthread[]> Worker;
	size_t WorkerCount;

public:

	/**
	 * @brief Create a thread pool.
	 * 
	 * @param thread_count The number of thread to hold.
	*/
	ThreadPool(size_t);

	~ThreadPool();

	//Enqueue a function for thread pool execution.
	//The first argument in the function receives a thread info structure.
	template<typename Func, typename... Arg, typename Ret = std::invoke_result_t<Func, ThreadInfo, Arg...>>
	[[nodiscard]] std::future<Ret> enqueue(Func&& func, Arg&&... arg) {
		using std::invoke;
		using namespace std::placeholders;

		const auto pm = std::make_shared<std::promise<Ret>>();
		{
			const auto lock = std::unique_lock(this->Mutex);
			this->Job.emplace([pm, user_func = std::bind(std::forward<Func>(func), _1, std::forward<Arg>(arg)...)]
			(const ThreadInfo& info) -> void {
				try {
					if constexpr (std::is_same_v<Ret, void>) {
						invoke(user_func, info);
						pm->set_value();
					} else {
						pm->set_value(invoke(user_func, info));
					}
				} catch (...) {
					pm->set_exception(std::current_exception());
				}
			});
		}
		this->Signal.notify_one();
		return pm->get_future();
	}

};

}