#pragma once

#include "ProcessThreadControl.hpp"
#include "Error.hpp"

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <tuple>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <thread>

#include <utility>

#include <cmath>
#include <cstddef>
#include <cstdint>

/**
 * @brief Control process and thread with POSIX API.
 */
namespace DisRegRep::Core::System::ProcessThreadControl::Posix {

using ThreadType = pthread_t;
using ProcessType = pid_t;

using std::tuple;
using std::for_each, std::execution::unseq,
	std::bind_front, std::mem_fn,
	std::views::iota, std::views::filter;
using std::jthread;

[[nodiscard]] inline auto getPriorityBound(const int policy) {
	const int priority_min = sched_get_priority_min(policy),
		priority_max = sched_get_priority_max(policy);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_max);
	return tuple(priority_min, priority_max);
}

[[nodiscard]] inline ThreadType getCurrentThread() noexcept {
	return pthread_self();
}

[[nodiscard]] inline ProcessType getCurrentProcess() noexcept {
	return getpid();
}

[[nodiscard]] inline Priority getNativeThreadPriority(const ThreadType thread) {
	int policy;
	sched_param param;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getschedparam(thread, &policy, &param));

	const auto [priority_min, priority_max] = getPriorityBound(policy);
	const float norm_priority = (1.0F * param.sched_priority - priority_min) / (priority_max - priority_min);
	return static_cast<Priority>(std::lerp(PriorityPreset::Min, PriorityPreset::Max, norm_priority));
}

inline void setNativeThreadPriority(const ThreadType thread, const Priority priority) {
	int policy;
	sched_param param;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getschedparam(thread, &policy, &param));
	(void)param;

	const auto [priority_min, priority_max] = getPriorityBound(policy);
	const float norm_priority = (1.0F * priority - PriorityPreset::Min) / (PriorityPreset::Max - PriorityPreset::Min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX(
		pthread_setschedprio(thread, static_cast<int>(std::lerp(priority_min, priority_max, norm_priority))));
}

[[nodiscard]] inline AffinityMask getNativeThreadAffinityMask(ProcessType, const ThreadType thread) {
	cpu_set_t set;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getaffinity_np(thread, sizeof(set), &set));

	AffinityMask mask;
	auto set_rg = iota(0, std::ranges::max(CPU_SETSIZE, static_cast<int>(mask.size())))
		| filter([&set](const int i) noexcept { return CPU_ISSET(i, &set); });
	for_each(unseq, set_rg.cbegin(), set_rg.cend(),
		bind_front(mem_fn(static_cast<AffinityMask& (AffinityMask::*)(std::size_t, bool)>(&AffinityMask::set)), std::ref(mask)));
	return mask;
}

inline void setNativeThreadAffinityMask(ProcessType, const ThreadType thread, const AffinityMask affinity_mask) {
	cpu_set_t set;
	CPU_ZERO(&set);

	const AffinityMask availability_mask = (std::uint64_t { 1 } << jthread::hardware_concurrency()) - 1U;
	auto mask = iota(0UZ, affinity_mask.size()) | filter(bind_front(mem_fn(&AffinityMask::test), affinity_mask & availability_mask));
	for_each(unseq, mask.cbegin(), mask.cend(), [&set](const int i) noexcept { CPU_SET(i, &set); });
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setaffinity_np(thread, sizeof(set), &set));
}

}