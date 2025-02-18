#pragma once

#include "../Exception.hpp"
#include "Platform.hpp"

#ifdef DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#include <Windows.h>
#include <errhandlingapi.h>
#endif

#include <format>

#include <exception>
#include <system_error>

#include <cerrno>

//Check if a Windows API call is successful.
#define DRR_ASSERT_SYSTEM_ERROR_WINDOWS(EXPR) \
	do { \
		try { \
			DRR_ASSERT(EXPR); \
		} catch (...) { \
			const DWORD last_error = GetLastError(); \
			std::throw_with_nested(std::system_error(last_error, std::system_category(), \
				std::format("Windows Error Code {}", last_error))); \
		} \
	} while (false)

//Check if a POSIX API call that returns an error code is successful.
#define DRR_ASSERT_SYSTEM_ERROR_POSIX(EXPR) \
	do { \
		try { \
			int ec; \
			DRR_ASSERT((ec = (EXPR), !ec)); \
		} catch (...) { \
			std::throw_with_nested(std::system_error(std::make_error_code(std::errc { ec }))); \
		} \
	} while (false)

//Similar to `DRR_ASSERT_SYSTEM_ERROR_POSIX`, but error code is set to `errno`.
#define DRR_ASSERT_SYSTEM_ERROR_POSIX_ERRNO(EXPR) \
	do { \
		try { \
			DRR_ASSERT((EXPR) != -1); \
		} catch (...) { \
			std::throw_with_nested(std::system_error(std::make_error_code(std::errc { errno }))); \
		} \
	} while (false)