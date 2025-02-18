#pragma once

#include "../Exception.hpp"
#include "Platform.hpp"

/**
 * @brief Error handling for system API call.
 */
namespace DisRegRep::Core::System::Error {

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
/**
 * @exception std::system_error with error code from @link GetLastError.
 */
[[noreturn]] void throwWindows();
#elifdef DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
/**
 * @param ec Error code.
 * 
 * @exception std::system_error with error code `ec`.
 */
[[noreturn]] void throwPosix(int);

/**
 * @exception std::system_error with error code taken from @link errno.
 */
[[noreturn]] void throwPosixErrno();
#endif

}

//Check if a Windows API call is successful.
#define DRR_ASSERT_SYSTEM_ERROR_WINDOWS(EXPR) \
	do { \
		try { \
			DRR_ASSERT(EXPR); \
		} catch (...) { \
			DisRegRep::Core::System::Error::throwWindows(); \
		} \
	} while (false)

//Check if a POSIX API call that returns an error code is successful.
#define DRR_ASSERT_SYSTEM_ERROR_POSIX(EXPR) \
	do { \
		int ec; \
		try { \
			DRR_ASSERT((ec = (EXPR), !ec)); \
		} catch (...) { \
			DisRegRep::Core::System::Error::throwPosix(ec); \
		} \
	} while (false)

//Similar to `DRR_ASSERT_SYSTEM_ERROR_POSIX`, but error code is set to `errno`.
#define DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(EXPR) \
	do { \
		try { \
			DRR_ASSERT((EXPR) != -1); \
		} catch (...) { \
			DisRegRep::Core::System::Error::throwPosixErrno(); \
		} \
	} while (false)