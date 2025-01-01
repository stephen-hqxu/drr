#pragma once

#include "System/ProcessThreadControl.hpp"

#include <queue>
#include <vector>

#include <functional>
#include <memory>

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <utility>

#include <concepts>
#include <type_traits>

#include <exception>

#include <cstdint>

namespace DisRegRep::Core {

/**
 * @brief A pool of reusable threads.
 */
class ThreadPool {
public:

	using SizeType = std::uint_fast8_t;

	/**
	 * @brief Additional information regarding the thread assigned for executing the task.
	 */
	struct ThreadInfo {

		SizeType Index; /**< Index of the thread within the living thread pool. */

	};

	//Type of `ThreadInfo` when passed through the task.
	using ThreadInfoArgumentType = std::add_lvalue_reference_t<std::add_const_t<ThreadInfo>>;

private:

	/**
	 * @brief A type-erased task submitted by the application.
	 */
	class AnyTask {
	public:

		constexpr AnyTask() noexcept = default;

		AnyTask(const AnyTask&) = delete;

		AnyTask(AnyTask&&) = delete;

		AnyTask& operator=(const AnyTask&) = delete;

		AnyTask& operator=(AnyTask&&) = delete;

		virtual constexpr ~AnyTask() = default;

		/**
		 * @brief Execute the task.
		 *
		 * @param thread_info The information regarding the executing thread.
		 */
		virtual void operator()(ThreadInfoArgumentType) = 0;

	};

	/**
	 * @brief A task submitted by the application.
	 *
	 * @tparam F Type of function represents the task.
	 */
	template<std::invocable<ThreadInfoArgumentType> F>
	class ApplicationTask final : public AnyTask {
	public:

		using TaskType = F;
		using ReturnType = std::invoke_result_t<TaskType, ThreadInfoArgumentType>;

		TaskType Task;
		std::promise<ReturnType> Promise;

		/**
		 * @brief Create an application task.
		 *
		 * @param task The wrapped function that takes a thread info as the first argument.
		 */
		explicit ApplicationTask(TaskType&& task) : Task(std::move(task)) { }

		~ApplicationTask() override = default;

		void operator()(ThreadInfoArgumentType thread_info) override {
			using std::invoke, std::is_same_v, std::current_exception;
			try {
				if constexpr (is_same_v<ReturnType, void>) {
					invoke(this->Task, thread_info);
					this->Promise.set_value();
				} else {
					this->Promise.set_value(invoke(this->Task, thread_info));
				}
			} catch (...) {
				this->Promise.set_exception(current_exception());
			}
		}

	};

	std::queue<std::unique_ptr<AnyTask>> TaskQueue;

	std::mutex Mutex;
	std::condition_variable Signal;

	std::vector<std::jthread> Thread;

public:

	/**
	 * @brief Create a thread pool.
	 *
	 * @param size Number of thread to be allocated to the pool.
	 */
	ThreadPool(SizeType);

	ThreadPool(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = delete;

	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool& operator=(ThreadPool&&) = delete;

	~ThreadPool();

	/**
	 * @brief Get thread pool size.
	 *
	 * @return Number of thread in the pool.
	 */
	[[nodiscard]] constexpr SizeType size() const noexcept {
		return this->Thread.size();
	}

	/**
	 * @brief Set priority of all threads in the pool.
	 *
	 * @param priority Priority value set to.
	 */
	void setPriority(System::ProcessThreadControl::Priority);

	/**
	 * @brief Set affinity mask of all threads in the pool.
	 *
	 * @param affinity_mask Affinity mask set to.
	 */
	void setAffinityMask(System::ProcessThreadControl::AffinityMask);

	/**
	 * @brief Enqueue an executable task to the thread pool.
	 *
	 * @tparam Arg Argument type of the task function.
	 * @tparam F Task function type.
	 *
	 * @param f Function that represents the task. The first argument receives info of the executing thread.
	 * @param arg Remaining arguments used for calling the function.
	 *
	 * @return Future.
	 */
	template<
		typename... Arg,
		std::invocable<ThreadInfoArgumentType, Arg...> F,
		typename ReturnType = std::invoke_result_t<F, ThreadInfoArgumentType, Arg...>
	>
	requires std::is_move_constructible_v<F>
	[[nodiscard]] std::future<ReturnType> enqueue(F&& f, Arg&&... arg) {
		using std::make_unique, std::bind_back;
		using std::unique_lock;

		auto bound = bind_back(std::forward<F>(f), std::forward<Arg>(arg)...);
		auto task = make_unique<ApplicationTask<decltype(bound)>>(std::move(bound));
		auto future = task->Promise.get_future();
		{
			const auto lock = unique_lock(this->Mutex);
			this->TaskQueue.push(std::move(task));
		}
		this->Signal.notify_one();
		return future;
	}

};

}