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

#include <type_traits>

#include <cmath>

/**
 * @brief Control process and thread with POSIX API.
 */
namespace DisRegRep::Core::System::ProcessThreadControl::Posix {

using HandleType = pthread_t;

using std::for_each, std::execution::unseq,
	std::mem_fn,
	std::views::iota, std::views::filter,
	std::ranges::input_range, std::ranges::range_value_t;
using std::is_arithmetic_v;

[[nodiscard]] inline auto toAbsolutePriority(const float norm_priority) {
	const int priority_min = sched_get_priority_min(policy),
		priority_max = sched_get_priority_max(policy);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_max);
	return static_cast<int>(std::lerp(priority_min, priority_max, norm_priority));
}

template<input_range Mask>
requires is_arithmetic_v<range_value_t<Mask>>
[[nodiscard]] cpu_set_t copyToCpuSet(Mask&& mask) noexcept {
	cpu_set_t set;
	CPU_ZERO(&set);

	using std::ranges::cbegin, std::ranges::cend;
	for_each(unseq, cbegin(mask), cend(mask), [&set](const int i) noexcept { CPU_SET(i, &set); });
	return set;
}

[[nodiscard]] inline HandleType getCurrentThread() noexcept {
	return pthread_self();
}

inline void setNativeThreadPriority(const HandleType thread, const Priority priority) {
	int policy;
	sched_param param;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getschedparam(thread, &policy, &param));
	(void)param;

	const float norm_priority = (1.0F * priority - PriorityPreset::Min) / (PriorityPreset::Max - PriorityPreset::Min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setschedprio(thread, toAbsolutePriority(norm_priority)));
}

inline void setNativeThreadPriority(const HandleType thread) {
	//There is no portal way to restore the default priority because it inherits from the calling thread at creation,
	//	but half of the priority range is usually a good assumption.
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setschedprio(thread, toAbsolutePriority(0.5F)));
}

inline void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	const cpu_set_t set = copyToCpuSet(iota(0UZ, affinity_mask.size()) | filter(mem_fn(&AffinityMask::test)));
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setaffinity_np(thread, sizeof(set), &set));
}

inline void setNativeThreadAffinityMask(const HandleType thread) {
	const cpu_set_t set = copyToCpuSet(iota(0, CPU_SETSIZE));
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setaffinity_np(thread, sizeof(set), &set));
}

}