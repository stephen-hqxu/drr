#pragma once

#include "ProcessThreadControl.hpp"
#include "Error.hpp"

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>

#include <cmath>

/**
 * @brief Control process and thread with POSIX API.
 */
namespace DisRegRep::Core::System::ProcessThreadControl::Posix {

using HandleType = pthread_t;

using std::for_each, std::execution::unseq,
	std::mem_fn,
	std::views::iota, std::views::filter;

[[nodiscard]] inline HandleType getCurrentThread() noexcept {
	return pthread_self();
}

inline void setNativeThreadPriority(const HandleType thread, const Priority priority) {
	int policy;
	sched_param param;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getschedparam(thread, &policy, &param));
	(void)param;

	const int priority_min = sched_get_priority_min(policy),
		priority_max = sched_get_priority_max(policy);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_max);

	const float norm_priority = (1.0F * priority - MinPriority) / (MaxPriority - MinPriority);
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setschedprio(thread, std::lerp(priority_min, priority_max, norm_priority)));
}

inline void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	cpu_set_t set;
	CPU_ZERO(&set);

	auto set_mask = iota(0UZ, affinity_mask.size()) | filter(mem_fn(&AffinityMask::test));
	for_each(unseq, set_mask.cbegin(), set_mask.cend(), [&set](const auto i) noexcept { CPU_SET(i, &set); });

	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setaffinity_np(thread, sizeof(set), &set));
}

}