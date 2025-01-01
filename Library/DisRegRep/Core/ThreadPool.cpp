#include <DisRegRep/Core/ThreadPool.hpp>

#include <DisRegRep/Core/System/ProcessThreadControl.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <range/v3/view/addressof.hpp>

#include <algorithm>
#include <functional>
#include <ranges>

#include <mutex>
#include <stop_token>
#include <thread>

#include <utility>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;
using DisRegRep::Core::ThreadPool;

using ranges::views::addressof;

using std::from_range, std::ranges::for_each,
	std::invoke, std::mem_fn, std::bind_front,
	std::views::iota, std::views::transform;
using std::unique_lock, std::defer_lock,
	std::jthread, std::stop_token;
using std::as_const;

ThreadPool::ThreadPool(const SizeType size) :
	Thread(from_range, iota(SizeType {}, size) | transform([this](const auto thread_idx) {
		return jthread(
			[this](const stop_token token, const ThreadInfo thread_info) {
				auto lock = unique_lock(this->Mutex, defer_lock);
				while (true) {
					lock.lock();
					this->Signal.wait(lock, [&task_queue = as_const(this->TaskQueue), &token]() noexcept {
						return !task_queue.empty() || token.stop_requested();
					});
					//Thread pool should not be killed until the task queue is empty.
					if (this->TaskQueue.empty() && token.stop_requested()) {
						return;
					}

					auto task = std::move(this->TaskQueue.front());
					this->TaskQueue.pop();

					lock.unlock();
					invoke(*task, thread_info);
				}
			}, ThreadInfo {
				.Index = thread_idx
			});
	})) {
	DRR_ASSERT(this->size() > 0U);
}

ThreadPool::~ThreadPool() {
	for_each(this->Thread, mem_fn(&jthread::request_stop));
	this->Signal.notify_all();
}

void ThreadPool::setPriority(const ProcThrCtrl::Priority priority) {
	for_each(this->Thread | addressof, bind_front(ProcThrCtrl::setPriority, priority));
}

void ThreadPool::setAffinityMask(const ProcThrCtrl::AffinityMask affinity_mask) {
	for_each(this->Thread | addressof, bind_front(ProcThrCtrl::setAffinityMask, affinity_mask));
}