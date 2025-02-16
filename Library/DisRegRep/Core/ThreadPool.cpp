#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/View/Functional.hpp>
#include <DisRegRep/Core/Exception.hpp>

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

using std::from_range, std::ranges::for_each,
	std::invoke, std::mem_fn, std::bind_front,
	std::views::iota, std::views::transform;
using std::unique_lock, std::shared_lock, std::defer_lock,
	std::jthread, std::stop_token;

ThreadPool::ThreadPool(const SizeType size) :
	Task {
		.Semaphore = decltype(this->Task.Semaphore)(0)
	},
	Thread(from_range, iota(SizeType {}, size) | transform([this](const auto thread_idx) {
		return jthread(
			[this](const stop_token token, const ThreadInfo thread_info) {
				auto& [queue, mutex, semaphore] = this->Task;

				auto lock = unique_lock(mutex, defer_lock);
				while (true) {
					semaphore.acquire();
					lock.lock();
					//Thread pool should not be killed until the task queue is empty.
					const auto no_task = queue.empty();
					if (no_task && token.stop_requested()) [[unlikely]] {
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
	//Once all tasks are finished, threads will go back to the waiting sequence.
	//Therefore, we need an additional wakeup so it can check for stopping status.
	this->Task.Semaphore.release(this->sizeThread());
}

ThreadPool::SizeType ThreadPool::sizeTask() const {
	const auto& [queue, mutex, _] = this->Task;
	const auto lock = shared_lock(mutex);
	return queue.size();
}

void ThreadPool::setPriority(const ProcThrCtrl::Priority priority) {
	for_each(this->Thread | View::Functional::AddressOf,
		bind_front(static_cast<void (*)(ProcThrCtrl::Priority, jthread*)>(ProcThrCtrl::setPriority), priority));
}

void ThreadPool::setAffinityMask(const ProcThrCtrl::AffinityMask affinity_mask) {
	for_each(this->Thread | View::Functional::AddressOf,
		bind_front(static_cast<void (*)(ProcThrCtrl::AffinityMask, jthread*)>(ProcThrCtrl::setAffinityMask), affinity_mask));
}