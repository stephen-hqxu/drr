#pragma once

#include "ProcessThreadControl.hpp"
#include "Error.hpp"

#include "../View/Arithmetic.hpp"

#include <Windows.h>
#include <WinBase.h>
#include <processthreadsapi.h>

#include <array>

#include <algorithm>
#include <iterator>

/**
 * @brief Control process and thread with Windows API.
 */
namespace DisRegRep::Core::System::ProcessThreadControl::Windows {

using ThreadType = HANDLE;
using ProcessType = HANDLE;

using std::array;
using std::ranges::is_sorted, std::ranges::lower_bound,
	std::ranges::distance, std::ranges::range_size_t;

inline constexpr array SystemPriorityProgression = {
	THREAD_PRIORITY_IDLE,
	THREAD_PRIORITY_LOWEST,
	THREAD_PRIORITY_BELOW_NORMAL,
	THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_ABOVE_NORMAL,
	THREAD_PRIORITY_HIGHEST
};
static_assert(is_sorted(SystemPriorityProgression));
inline constexpr auto PortablePriorityProgression =
	View::Arithmetic::LinSpace(PriorityPreset::Min, PriorityPreset::Max - 1U, SystemPriorityProgression.size());

[[nodiscard]] inline ThreadType getCurrentThread() noexcept {
	return GetCurrentThread();
}

[[nodiscard]] inline ProcessType getCurrentProcess() noexcept {
	return GetCurrentProcess();
}

[[nodiscard]] inline DWORD_PTR getProcessAffinityMask(const ProcessType process) {
	DWORD_PTR process_affinity_mask, system_affinity_mask;
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(GetProcessAffinityMask(process, &process_affinity_mask, &system_affinity_mask));
	(void)system_affinity_mask;
	return process_affinity_mask;
}

[[nodiscard]] inline Priority getNativeThreadPriority(const ThreadType thread) {
	const int priority = GetThreadPriority(thread);
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(priority != THREAD_PRIORITY_ERROR_RETURN);

	const auto priority_it = lower_bound(SystemPriorityProgression, priority);
	return priority_it == SystemPriorityProgression.cend()
		? PortablePriorityProgression.back()
		: PortablePriorityProgression[static_cast<range_size_t<decltype(PortablePriorityProgression)>>(
			distance(SystemPriorityProgression.cbegin(), priority_it))];
}

inline void setNativeThreadPriority(const ThreadType thread, const Priority priority) {
	const auto priority_band_it = lower_bound(PortablePriorityProgression, priority);
	const int system_priority = priority_band_it == PortablePriorityProgression.cend()
		? THREAD_PRIORITY_TIME_CRITICAL
		: SystemPriorityProgression[static_cast<decltype(SystemPriorityProgression)::size_type>(
			distance(PortablePriorityProgression.cbegin(), priority_band_it))];
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadPriority(thread, system_priority));
}

[[nodiscard]] inline AffinityMask getNativeThreadAffinityMask(const ProcessType process, const ThreadType thread) {
	const DWORD_PTR process_affinity_mask = getProcessAffinityMask(process),
		old = SetThreadAffinityMask(thread, process_affinity_mask);
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(old);
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, old) == process_affinity_mask);
	return old;
}

inline void setNativeThreadAffinityMask(const ProcessType process, const ThreadType thread, const AffinityMask affinity_mask) {
	const DWORD_PTR process_affinity_mask = getProcessAffinityMask(process);
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, process_affinity_mask & affinity_mask.to_ullong()));
}

}