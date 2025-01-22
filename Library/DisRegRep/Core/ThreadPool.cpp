#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/Exception.hpp>
#include <DisRegRep/Core/Range.hpp>

#include <range/v3/view/addressof.hpp>
#include <range/v3/view/iota.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <thread>

#include <utility>

#include <cassert>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;
using DisRegRep::Core::ThreadPool;

using ranges::views::addressof, ranges::views::iota;

using std::from_range, std::ranges::for_each,
	std::invoke, std::mem_fn, std::bind_front;
using std::unique_lock, std::shared_lock, std::defer_lock,
	std::jthread, std::stop_token;

ThreadPool::ThreadPool(const SizeType size) :
	Task {
		.Semaphore = decltype(this->Task.Semaphore)(0)
	},
	Thread(from_range, iota(SizeType {}, size) | Range::ImpureTransform([this](const auto thread_idx) {
		return jthread(
			[this](const stop_token token, const ThreadInfo thread_info) {
				auto& [queue, mutex, semaphore] = this->Task;

				auto lock = unique_lock(mutex, defer_lock);
				while (true) {
					semaphore.acquire();
					lock.lock();
					//Thread pool should not be killed until the task queue is empty.
					const auto no_task = queue.empty();
					if (no_task && token.stop_requested()) {
						return;
					}
					assert(!no_task);

					auto task = std::move(queue.front());
					queue.pop();

					lock.unlock();
					invoke(*task, thread_info);
				}
			}, ThreadInfo {
				.Index = thread_idx
			});
	})) {
	DRR_ASSERT(this->sizeThread() > 0U);
}

ThreadPool::~ThreadPool() {
	for_each(this->Thread, mem_fn(&jthread::request_stop));
	//Reason why both sizes are added is because a thread needs to carry on the execution until all task queue is exhausted.
	//However, once a task is finished, the thread will go back to the waiting sequence.
	//Therefore, we need an additional wakeup so it can check for stopping status.
	//To put it simple, task execution and thread stopping are two disjoint operations that we need to signal separately.
	this->Task.Semaphore.release(this->sizeThread() + this->sizeTask());
}

ThreadPool::SizeType ThreadPool::sizeTask() const {
	const auto& [queue, mutex, _] = this->Task;
	const auto lock = shared_lock(mutex);
	return queue.size();
}

void ThreadPool::setPriority(const ProcThrCtrl::Priority priority) {
	for_each(this->Thread | addressof, bind_front(ProcThrCtrl::setPriority, priority));
}

void ThreadPool::setAffinityMask(const ProcThrCtrl::AffinityMask affinity_mask) {
	for_each(this->Thread | addressof, bind_front(ProcThrCtrl::setAffinityMask, affinity_mask));
}