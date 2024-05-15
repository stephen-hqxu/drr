#include <DisRegRep/Launch/System/ProcessThreadControl.hpp>
#include <DisRegRep/Launch/System/Platform.hpp>

#if DRR_PLATFORM_OS & (DRR_PLATFORM_OS_WIN32 | DRR_PLATFORM_OS_WIN64)
#define USE_WINDOWS_PROCESS_THREAD
#elif DRR_PLATFORM_OS & (DRR_PLATFORM_OS_CYGWIN | DRR_PLATFORM_OS_POSIX)
#define USE_POSIX_PROCESS_THREAD
#else
#error Cannot control process and thread because the code is being compiled on a unsupported platform.
#endif

#ifdef USE_WINDOWS_PROCESS_THREAD
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>
#elifdef USE_POSIX_PROCESS_THREAD
#include <pthread.h>
#include <sched.h>
#endif

#include <algorithm>

#include <format>
#include <source_location>
#include <stdexcept>

#include <limits>
#include <type_traits>
#include <utility>

#include <cmath>
#include <cerrno>

using std::numeric_limits, std::to_underlying, std::underlying_type_t;
using std::runtime_error, std::format, std::source_location;

using namespace DisRegRep;
using ProcessThreadControl::Priority;

namespace {

#ifdef USE_WINDOWS_PROCESS_THREAD
HANDLE getNativeThread() noexcept {
	return GetCurrentThread();
}

void setNativeThreadPriority(HANDLE thread_handle, const Priority priority) {
	constexpr static auto translate_priority = [](const Priority priority) constexpr static noexcept -> int {
		using enum Priority;
		switch (priority) {
		case Min: return THREAD_PRIORITY_IDLE;
		case Low: return THREAD_PRIORITY_LOWEST;
		case Medium: return THREAD_PRIORITY_NORMAL;
		case High: return THREAD_PRIORITY_HIGHEST;
		//CONSIDER: Maybe to change the process class to real-time? But that requires root privilege.
		case Max: return THREAD_PRIORITY_TIME_CRITICAL;
		default: std::unreachable();
		}
	};
	if (!SetThreadPriority(thread_handle, translate_priority(priority))) {
		throw runtime_error(format("Failed to set thread priority on Windows, given error code: {}.", GetLastError()));
	}
}
//^^^ Windows ^^^ // vvv POSIX vvv
#elifdef USE_POSIX_PROCESS_THREAD
//REMINDER: The implementation for setting pthread priority is experimental.
//I am not familiar with pthread and I don't have a working compiler on POSIX system to compile this codebase, so everything here
//	untested; it may not even compile!
void checkPthread(const int ret, const source_location src = source_location::current()) {
	if (ret) {
		throw runtime_error(
			format("Error from pthread at line {} from function {}, given error code: {}.", src.line(), src.function_name(), ret));
	}
}

pthread_t getNativeThread() noexcept {
	return pthread_self();
}

void setNativeThreadPriority(const pthread_t pt, const Priority priority) {
	constexpr static auto translate_priority = [](const Priority priority, const int min,
												   const int max) static noexcept -> int {
		const float ratio = 1.0F * to_underlying(priority) / numeric_limits<underlying_type_t<Priority>>::max(),
			distance = max - min,
			rescaled = ratio * distance;
		return std::clamp(static_cast<int>(std::round(min + rescaled)), min, max);
	};
	//CONSIDER: Instead of overwriting the policy, get the current policy of the given thread.
	constexpr static int policy = SCHED_OTHER;

	const int prio_min = sched_get_priority_min(policy), prio_max = sched_get_priority_max(policy);
	if (prio_min < 0 || prio_max < 0) {
		checkPthread(errno);
	}

	const sched_param param {
		.sched_priority = translate_priority(priority, prio_min, prio_max)
	};
	checkPthread(pthread_setschedparam(pt, policy, &param));
}
#endif

}

void ProcessThreadControl::setPriority(std::jthread& thread, const Priority priority) {
	setNativeThreadPriority(thread.native_handle(), priority);
}

void ProcessThreadControl::setPriority(const Priority priority) {
	setNativeThreadPriority(getNativeThread(), priority);
}