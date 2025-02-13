#pragma once

#include "ProcessThreadControl.hpp"
#include "Error.hpp"

#include "../View/Arithmetic.hpp"

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
		View::Arithmetic::LinSpace(PriorityPreset::Min, PriorityPreset::Max - 1U, SystemPriorityProgression.size());

	const auto priority_band_it = find_if(PriorityValueProgression, bind_front(less_equal {}, priority));
	const int system_priority = priority_band_it == PriorityValueProgression.end()
		? THREAD_PRIORITY_TIME_CRITICAL
		: SystemPriorityProgression[
			static_cast<decltype(SystemPriorityProgression)::size_type>(distance(PriorityValueProgression.begin(), priority_band_it))];
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadPriority(thread, system_priority));
}

inline void setNativeThreadPriority(const HandleType thread) {
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadPriority(thread, THREAD_PRIORITY_NORMAL));
}

inline void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, affinity_mask.to_ullong()));
}

inline void setNativeThreadAffinityMask(const HandleType thread) {
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, DWORD_PTR {}));
}

}