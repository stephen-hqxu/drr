#pragma once

/********************
 * Operating System *
 ********************/
#if defined(_WIN32) || defined(_WIN64)
#define DRR_CORE_SYSTEM_PLATFORM_OS_WINDOWS
#elifdef _POSIX_VERSION
#define DRR_CORE_SYSTEM_PLATFORM_OS_POSIX
#else
#error Platform is unsupported.
#endif

/************
 * Compiler *
 ************/
#ifdef __GNUC__
#define DRR_CORE_SYSTEM_PLATFORM_COMPILER_GNU
#elifdef __clang__
#define DRR_CORE_SYSTEM_PLATFORM_COMPILER_CLANG
#else
#error Compiler is unsupported.
#endif

/***********
 * Warning *
 ***********/
#ifdef DRR_CORE_SYSTEM_PLATFORM_COMPILER_CLANG
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_PUSH _Pragma("clang diagnostic push")
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_IGNORE_CLANG(VALUE) _Pragma("clang diagnostic ignored " #VALUE)
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_POP _Pragma("clang diagnostic pop")
#endif

#ifndef DRR_CORE_SYSTEM_PLATFORM_WARNING_PUSH
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_PUSH
#endif
#ifndef DRR_CORE_SYSTEM_PLATFORM_WARNING_IGNORE_CLANG
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_IGNORE_CLANG(...)
#endif
#ifndef DRR_CORE_SYSTEM_PLATFORM_WARNING_POP
#define DRR_CORE_SYSTEM_PLATFORM_WARNING_POP
#endif