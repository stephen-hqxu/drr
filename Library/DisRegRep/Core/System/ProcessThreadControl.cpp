#include <DisRegRep/Core/System/ProcessThreadControl.hpp>
#include <DisRegRep/Core/System/Error.hpp>
#include <DisRegRep/Core/System/Platform.hpp>

#include <DisRegRep/Core/Exception.hpp>

#include <range/v3/view/linear_distribute.hpp>

//NOLINTBEGIN(misc-include-cleaner, misc-unused-using-decls)
/**********
 * OS API *
 **********/
#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#include <Windows.h>
#include <WinBase.h>
#include <processthreadsapi.h>
#elifdef DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#endif
/*****************************************************************************/

#include <array>

#include <algorithm>
#include <execution>
#include <functional>
#include <iterator>
#include <ranges>

#include <thread>

#include <cmath>

namespace ProcThrCtrl = DisRegRep::Core::System::ProcessThreadControl;
using ProcThrCtrl::Priority, ProcThrCtrl::AffinityMask;

using ranges::views::linear_distribute;

using std::array;
using std::for_each, std::execution::unseq,
	std::ranges::find_if,
	std::ranges::less_equal, std::bind_front,
	std::ranges::distance;
using std::views::iota, std::views::filter;
using std::jthread;
//NOLINTEND(misc-include-cleaner, misc-unused-using-decls)

namespace {

using HandleType = jthread::native_handle_type;

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
[[nodiscard]] HandleType getCurrentThread() noexcept {
	return GetCurrentThread();
}

void setNativeThreadPriority(const HandleType thread, const Priority priority) {
	static constexpr array SystemPriorityProgression = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST
	};
	static constexpr auto PriorityValueProgression =
		linear_distribute(ProcThrCtrl::MinPriority, Priority { ProcThrCtrl::MaxPriority - 1 }, SystemPriorityProgression.size());

	const auto priority_band_it = find_if(PriorityValueProgression, bind_front(less_equal {}, priority));
	const int system_priority = priority_band_it == PriorityValueProgression.end()
								  ? THREAD_PRIORITY_TIME_CRITICAL
								  : SystemPriorityProgression[distance(PriorityValueProgression.begin(), priority_band_it)];
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadPriority(thread, system_priority));
}

void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	DRR_ASSERT_SYSTEM_ERROR_WINDOWS(SetThreadAffinityMask(thread, affinity_mask.to_ullong()));
}
#elifdef DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
//^^^ Windows ^^^ // vvv POSIX vvv
[[nodiscard]] HandleType getCurrentThread() noexcept {
	return pthread_self();
}

void setNativeThreadPriority(const HandleType thread, const Priority priority) {
	int policy;
	sched_param param;
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_getschedparam(thread, &policy, &param));
	(void)param;

	const int priority_min = sched_get_priority_min(policy),
		priority_max = sched_get_priority_max(policy);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_min);
	DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(priority_max);

	const float norm_priority = (1.0F * priority - ProcThrCtrl::MinPriority) / (ProcThrCtrl::MaxPriority - ProcThrCtrl::MinPriority);
	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setschedprio(thread, std::lerp(priority_min, priority_max, norm_priority)));
}

void setNativeThreadAffinityMask(const HandleType thread, const AffinityMask affinity_mask) {
	cpu_set_t set;
	CPU_ZERO(&set);

	auto set_mask =
		iota(0UZ, affinity_mask.size()) | filter([affinity_mask](const auto i) constexpr noexcept { return affinity_mask.test(i); });
	for_each(unseq, set_mask.cbegin(), set_mask.cend(), [&set](const auto i) noexcept { CPU_SET(i, &set); });

	DRR_ASSERT_SYSTEM_ERROR_POSIX(pthread_setaffinity_np(thread, sizeof(set), &set));
}
#endif
//^^^ POSIX ^^^ // vvv Platform Independent vvv
[[nodiscard]] HandleType getNativeHandle(jthread* const thread) noexcept {
	return thread ? thread->native_handle() : getCurrentThread();
}

}

void ProcThrCtrl::setPriority(const Priority priority, jthread* const thread) {
	DRR_ASSERT(priority >= ProcThrCtrl::MinPriority && priority <= ProcThrCtrl::MaxPriority);
	setNativeThreadPriority(getNativeHandle(thread), priority);
}

void ProcThrCtrl::setAffinityMask(const AffinityMask affinity_mask, jthread* const thread) {
	DRR_ASSERT(jthread::hardware_concurrency() <= affinity_mask.size());
	setNativeThreadAffinityMask(getNativeHandle(thread), affinity_mask);
}