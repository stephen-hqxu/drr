#pragma once

#include "System/ProcessThreadControl.hpp"

#include <functional>
#include <memory>
#include <queue>

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <concepts>
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

	/**
	 * @brief A type-erased job submitted by the application.
	 */
	class JobEntry {
	public:

		constexpr JobEntry() noexcept = default;

		constexpr virtual ~JobEntry() = default;

		/**
		 * @brief Execute the job.
		 *
		 * @param thread_info The information regarding the executing thread.
		 */
		virtual void execute(const ThreadInfo&) = 0;

	};

	/**
	 * @brief The job entry submitted by the user.
	 *
	 * @tparam Func The function to be executed.
	 */
	template<typename Func>
	class UserJobEntry final : public JobEntry {
	public:

		using return_type = std::invoke_result_t<Func, ThreadInfo>;

		std::promise<return_type> Promise;
		Func Function;

		/**
		 * @brief Initialise a user job entry.
		 *
		 * @tparam T The type of the wrapped function.
		 * @param func The wrapped function that takes a thread info as the first argument.
		 */
		template<std::same_as<Func> T>
		UserJobEntry(T&& func) noexcept(std::is_nothrow_move_constructible_v<Func>) : Function(std::forward<T>(func)) { }

		~UserJobEntry() override = default;

		void execute(const ThreadInfo& thread_info) override {
			using std::invoke;
			try {
				if constexpr (std::is_same_v<return_type, void>) {
					invoke(this->Function, thread_info);
					this->Promise.set_value();
				} else {
					this->Promise.set_value(invoke(this->Function, thread_info));
				}
			} catch (...) {
				this->Promise.set_exception(std::current_exception());
			}
		}

	};

	std::queue<std::unique_ptr<JobEntry>> Job;

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
	explicit ThreadPool(size_t);

	ThreadPool(ThreadPool&&) = delete;

	ThreadPool& operator=(ThreadPool&&) = delete;

	~ThreadPool();

	/**
	 * @brief Set the priority for all threads in the thread pool.
	 *
	 * @param priority The priority value for all threads.
	 */
	void setPriority(ProcessThreadControl::Priority) const;

	//Enqueue a function for thread pool execution.
	//The first argument in the function receives a thread info structure.
	template<typename Func, typename... Arg, typename Ret = std::invoke_result_t<Func, ThreadInfo, Arg...>>
	[[nodiscard]] std::future<Ret> enqueue(Func&& func, Arg&&... arg) {
		using namespace std::placeholders;

		std::future<Ret> future;
		{
			const auto lock = std::unique_lock(this->Mutex);

			auto user_job = std::make_unique<
				UserJobEntry<decltype(std::bind(std::forward<Func>(func), _1, std::forward<Arg>(arg)...))>
			>(std::bind(std::forward<Func>(func), _1, std::forward<Arg>(arg)...));
			future = user_job->Promise.get_future();
			this->Job.emplace(std::move(user_job));
		}
		this->Signal.notify_one();
		return future;
	}

};

}