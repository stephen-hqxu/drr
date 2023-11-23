#pragma once

#include <DisRegRep/ProgramExport.hpp>

#include <queue>
#include <functional>

#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

#include <utility>
#include <exception>
#include <type_traits>

namespace DisRegRep {

/**
 * @brief A pool of reusable threads.
*/
class DRR_API ThreadPool {
public:

	/**
	 * @brief Additional information regarding the thread used to execute the job.
	*/
	struct ThreadInfo {

		size_t ThreadIndex;/**< Index of the thread within the thread pool. */

	};

private:

	std::queue<std::function<void(const ThreadInfo&)>> Job;

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
		using std::forward, std::invoke;
		using namespace std::placeholders;

		const auto pm = std::make_shared_for_overwrite<std::promise<Ret>>();
		{
			const auto lock = std::unique_lock(this->Mutex);
			this->Job.emplace([pm, user_func = std::bind(forward<Func>(func), _1, forward<Arg>(arg)...)]
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