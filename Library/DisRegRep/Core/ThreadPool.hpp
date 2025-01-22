#pragma once

#include "System/ProcessThreadControl.hpp"

#include <array>
#include <queue>
#include <tuple>
#include <vector>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>

#include <future>
#include <mutex>
#include <semaphore>
#include <shared_mutex>
#include <thread>

#include <memory>
#include <utility>

#include <concepts>
#include <type_traits>

#include <exception>

#include <cstddef>
#include <cstdint>

namespace DisRegRep::Core {

/**
 * @brief A pool of reusable threads.
 */
class ThreadPool {
public:

	using SizeType = std::uint_fast32_t;

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

	/**
	 * @brief Trait of an application task.
	 */
	template<typename>
	struct ApplicationTaskTrait;

	template<typename... Task>
	requires std::conjunction_v<std::conjunction<
		std::is_invocable<Task, ThreadInfoArgumentType>,
		std::is_copy_constructible<Task>,
		std::is_move_constructible<Task>
	>...>
	struct ApplicationTaskTrait<std::tuple<Task...>> {

		static constexpr std::size_t TaskSize = sizeof...(Task);

		using InvokeResult = std::tuple<std::type_identity<std::invoke_result_t<Task, ThreadInfoArgumentType>>...>;
		using InvokeResultTrait = decltype(std::apply(
			[]<typename Result0, typename... Result>(std::type_identity<Result0>, std::type_identity<Result>...) {
				using std::make_tuple, std::tuple, std::future,
					std::bool_constant, std::conjunction_v, std::is_same, std::conditional_t;

				static constexpr bool Identical = conjunction_v<is_same<Result0, Result>...>;
				using Future = conditional_t<Identical,
					future<Result0>,
					tuple<future<Result0>, future<Result>...>
				>;

				return make_tuple(
					bool_constant<Identical> {},
					Future {}
				);
			}, InvokeResult {}));
		template<std::size_t I>
		using InvokeResultTraitElement = std::tuple_element_t<I, InvokeResultTrait>;

		static constexpr bool IdenticalInvokeResult = InvokeResultTraitElement<0U>::value;
		using Future = InvokeResultTraitElement<1U>;

	};

	struct {

		std::queue<std::unique_ptr<AnyTask>> Queue;

		mutable std::shared_mutex Mutex;
		std::counting_semaphore<> Semaphore;

	} Task;

	std::vector<std::jthread> Thread;

public:

	/**
	 * @brief Create a thread pool.
	 *
	 * @param size Number of thread to be allocated to the pool.
	 */
	explicit ThreadPool(SizeType);

	ThreadPool(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = delete;

	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool& operator=(ThreadPool&&) = delete;

	/**
	 * @brief Wait for all queued tasks to finished before thread pool will be destroyed. The behaviour is undefined if more tasks are
	 * enqueued during this final clean-up stage.
	 */
	~ThreadPool();

	/**
	 * @brief Get thread pool size.
	 *
	 * @return Number of thread in the pool.
	 */
	[[nodiscard]] constexpr SizeType sizeThread() const noexcept {
		return this->Thread.size();
	}

	/**
	 * @brief Get number of task in the queue.
	 *
	 * @return Number of task in the queue.
	 */
	[[nodiscard]] SizeType sizeTask() const;

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
	 * @brief Enqueue executable tasks to the thread pool.
	 *
	 * @tparam TaskRange A range of tuples of task function types.
	 *
	 * @param task_range Range of tuples of functions where each represents the task. The first argument of all functions receives info
	 * of the executing thread.
	 * @param out_it Iterator to receive tuples of futures for each task enqueued in order, or flattened futures (without tuple) if all
	 * task functions return the same type.
	 */
	template<std::ranges::input_range TaskRange, typename TaskTrait = ApplicationTaskTrait<std::ranges::range_value_t<TaskRange>>>
	void enqueue(TaskRange&& task_range, const std::output_iterator<typename TaskTrait::Future> auto out_it) {
		using std::vector, std::array, std::to_array,
			std::make_tuple, std::apply,
			std::ranges::to, std::views::as_rvalue, std::views::transform, std::views::join,
			std::make_unique, std::unique_lock;
		static constexpr auto getFuture = [](const auto& task) static -> auto {
			return task->Promise.get_future();
		};

		auto& [queue, mutex, semaphore] = this->Task;
		//Not a mistake, we make a copy of each task.
		auto task = std::forward<TaskRange>(task_range) | transform([](auto task_tuple) static {
			return apply([]<typename... Task>(
							 Task&... task) static { return make_tuple(make_unique<ApplicationTask<Task>>(std::move(task))...); },
				task_tuple);
		}) | to<vector>();

		if constexpr (TaskTrait::IdenticalInvokeResult) {
			using std::ranges::copy;
			copy(task | transform([](const auto& app_task_tuple) static {
				return apply([](const auto&... task) static { return array { getFuture(task)... }; }, app_task_tuple);
			}) | join | as_rvalue,
				out_it);
		} else {
			using std::ranges::transform;
			transform(task, out_it, [](const auto& app_task_tuple) static {
				return apply([](const auto&... task) static { return make_tuple(getFuture(task)...); }, app_task_tuple);
			});
		}
		{
			const auto lock = unique_lock(mutex);
			queue.push_range(task | as_rvalue | transform([](auto app_task_tuple) static {
				return apply([](auto&... task) static { return to_array<decltype(queue)::value_type>({ std::move(task)... }); },
					app_task_tuple);
			}) | join | as_rvalue);
		}
		//Task container size should remain unchanged because we only moved each element.
		semaphore.release(task.size() * TaskTrait::TaskSize);
	}

};

}