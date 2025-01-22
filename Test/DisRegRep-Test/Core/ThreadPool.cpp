#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>

#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <optional>
#include <tuple>

#include <ranges>

#include <future>
#include <semaphore>

#include <utility>

#include <type_traits>

#include <cstdint>

using DisRegRep::Core::ThreadPool, DisRegRep::Core::System::ProcessThreadControl::Priority;

using Catch::Matchers::RangeEquals;

using std::vector,
	std::optional, std::tuple;
using std::views::transform, std::views::single,
	std::ranges::input_range, std::ranges::sized_range, std::ranges::range_value_t;
using std::future,
	std::counting_semaphore;
using std::is_same_v;

//NOLINTBEGIN(bugprone-unchecked-optional-access)
namespace {

//Those fabricated tasks are pretty lightweight, so don't bother taking too much system resources.
constexpr Priority DefaultPriority = 10U;

using Semaphore = counting_semaphore<16U>;
using TaskValue = std::int_least16_t;

class TestThreadPool {
protected:

	using SizeType = ThreadPool::SizeType;

	static constexpr SizeType ThreadSize = 2U;

	mutable Semaphore Semaphore_;
	mutable struct {

		optional<ThreadPool> Task;
		ThreadPool Destroyer;

	} ThreadPool_;

	TestThreadPool() : Semaphore_(0), ThreadPool_ { .Destroyer = ThreadPool(1U) } {
		auto& [_, destroyer_thread_pool] = this->ThreadPool_;
		destroyer_thread_pool.setPriority(DefaultPriority);
	}

	~TestThreadPool() = default;

	template<
		input_range FutureRg,
		input_range Reference,
		typename Future = range_value_t<FutureRg>,
		typename ReferenceValue = range_value_t<Reference>
	>
	requires sized_range<FutureRg> && is_same_v<Future, future<decltype(std::declval<Future>().get())>>
		  && is_same_v<ReferenceValue, TaskValue>
	void checkTask(FutureRg&& future_rg, Reference&& reference) const {
		using std::ranges::size;
		this->Semaphore_.release(size(future_rg));

		CHECK_THAT(std::forward<FutureRg>(future_rg) | transform([](Future& ft) static {
			ReferenceValue value;
			REQUIRE_NOTHROW(value = ft.get());
			return value;
		}), RangeEquals(std::forward<Reference>(reference)));
	}

	void tryInitialise() const {
		if (auto& task_thread_pool = this->ThreadPool_.Task;
			!task_thread_pool) [[unlikely]] {
			task_thread_pool.emplace(TestThreadPool::ThreadSize);
			task_thread_pool->setPriority(DefaultPriority);
		}
	}

	[[nodiscard]] auto destroy() const {
		auto ft = single(future<void>());
		this->ThreadPool_.Destroyer.enqueue(single(tuple([&task_thread_pool = this->ThreadPool_.Task](
			ThreadPool::ThreadInfoArgumentType) { task_thread_pool.reset(); })), ft.begin());
		return ft;
	}

	template<input_range Task, typename Value = range_value_t<Task>>
	requires sized_range<Task> && is_same_v<Value, TaskValue>
	[[nodiscard]] auto enqueue(Task&& task) const {
		using std::ranges::size;

		auto ft = vector<future<Value>>(size(task));
		this->ThreadPool_.Task->enqueue(
			std::forward<Task>(task) | transform([&semaphore = this->Semaphore_](const auto value) constexpr noexcept {
				return tuple([&semaphore, value](ThreadPool::ThreadInfoArgumentType) {
					semaphore.acquire();
					return value;
				});
			}),
			ft.begin());
		return ft;
	}

};

}

TEST_CASE_PERSISTENT_FIXTURE(TestThreadPool, "Thread pool queues tasks on a fixed number of reusable threads", "[Core][ThreadPool]") {
	const auto& [_1, thread_pool] = *this;
	const auto& [task_thread_pool, _2] = thread_pool;

	GIVEN("A thread pool initialised with a fixed number of threads") {
		this->tryInitialise();

		THEN("Number of threads initialised is the same as specified") {
			REQUIRE(task_thread_pool->sizeThread() == TestThreadPool::ThreadSize);
		}

		THEN("It has not task in the queue") {
			REQUIRE(task_thread_pool->sizeTask() == 0U);
		}

		WHEN("Some tasks are enqueued to the thread pool") {
			const auto task_value = GENERATE(take(5U, chunk(8U, random<TaskValue>(-2500, 2500))));
			const auto task_size = task_value.size();

			auto task_future = this->enqueue(task_value);

			THEN("Queue has tasks") {
				//There's a race condition here (though outcomes are finite and predictable).
				//Basically tasks may have already been grabbed by a thread, and they no longer stay in the queue.
				{
					const TestThreadPool::SizeType pending_task = task_thread_pool->sizeTask();
					CHECK((pending_task <= task_size && task_size <= pending_task + TestThreadPool::ThreadSize));
				}

				AND_THEN("Tasks executes, completes and returns results cleanly") {
					this->checkTask(task_future, task_value);
				}

				AND_WHEN("Thread pool is destroyed before tasks finish execution") {
					auto destroy_future = this->destroy();

					THEN("Thread pool waits for tasks to finish before initiating self-destruction") {
						//CONSIDER: It would be the best to check if thread pool is in pending destruction state.
						//I haven't figured out a good way of doing it.
						//There is a race condition if we only check the optional value-ness.
						//Using a lock will cause deadlock, since destruction never completes before tasks.
						//Did consider to set an atomic flag before destruction, but that will still give a race condition
						//	because there is a gap between setting the flag and starting destruction.
						//It's totally possible the destruction procedure is scheduled after tasks started.
						this->checkTask(task_future, task_value);

						REQUIRE_NOTHROW(destroy_future.front().get());
						CHECK_FALSE(task_thread_pool);
					}

				}

			}

		}

	}

}
//NOLINTEND(bugprone-unchecked-optional-access)