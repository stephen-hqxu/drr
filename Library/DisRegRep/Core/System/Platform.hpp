#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#elifdef _POSIX_VERSION
#define DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
#else
#error Platform is unsupported.
#endif