#pragma once

#include "ProcessThreadControl.hpp"
#include "Error.hpp"

#include <range/v3/view/linear_distribute.hpp>

#include <Windows.h>
#include <WinBase.h>
#include <processthreadsapi.h>

#include <array>

#include <algorithm>
#include <functional>
#include <iterator>

/**
 * @brief Control process and thread with Windows API.
 */
namespace DisRegRep::Core::System::ProcessThreadControl::Windows {

using HandleType = HANDLE;

using ranges::views::linear_distribute;

using std::array;
using std::ranges::find_if,
	std::bind_front, std::ranges::less_equal,
	std::ranges::distance;

[[nodiscard]] inline HandleType getCurrentThread() noexcept {
	return GetCurrentThread();
}

inline void setNativeThreadPriority(const HandleType thread, const Priority priority) {
	static constexpr array SystemPriorityProgression = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST
	};
	static constexpr auto PriorityValueProgression =
		linear_distribute(MinPriority, Priority { MaxPriority - 1 }, SystemPriorityProgression.size());

	const auto priority_band_it = find_if(PriorityValueProgression, bind_front(less_equal {}, priority));
	const int system_priority = priority_band_it == PriorityValueProgression.end()
								  ? THREAD_PRIORITY_TIME_CRITICAL
								  : SystemPriorityProgression[distance(PriorityValueProgression.begin(), priority_band_it)];
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadPriority(thread, system_priority));
}

inline void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, affinity_mask.to_ullong()));
}

}