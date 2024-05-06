#include <DisRegRep/Launch/ThreadPool.hpp>

#include <algorithm>
#include <ranges>

using std::jthread, std::stop_token, std::unique_lock;

using namespace DisRegRep;

ThreadPool::ThreadPool(const size_t thread_count) :
	Worker(std::make_unique_for_overwrite<jthread[]>(thread_count)), WorkerCount(thread_count) {
	std::ranges::transform(std::views::iota(size_t { 0 }, thread_count), this->Worker.get(), [this](const auto thread_idx) {
		return jthread([this](const stop_token should_stop, const size_t thread_idx) {
			const auto info = ThreadInfo {
				.ThreadIndex = thread_idx
			};

			auto lock = unique_lock(this->Mutex, std::defer_lock);
			while (true) {
				lock.lock();
				this->Signal.wait(lock,
					[&job = this->Job, &should_stop]() noexcept { return !job.empty() || should_stop.stop_requested(); });
				if (this->Job.empty() && should_stop.stop_requested()) {
					return;
				}

				auto task = std::move(this->Job.front());
				this->Job.pop();

				lock.unlock();
				std::invoke(task, info);
			}
		}, thread_idx);
	});
}

ThreadPool::~ThreadPool() {
	std::ranges::for_each_n(this->Worker.get(), this->WorkerCount, [](auto& thread) static { thread.request_stop(); });
	this->Signal.notify_all();
}