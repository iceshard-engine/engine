#pragma once


#define HW_WIN             0
#define HW_MAC            0
#define HW_LINUX        0
#define HW_DURANGO        0
#define HW_ORBIS        0

#ifdef __PRETTY_FUNCTION__
#define __PRETTYFUNCTION__ __PRETTY_FUNCTION__
#else
#define __PRETTYFUNCTION__ __FUNCTION__
#endif


// For windows systems
#if _WIN64 || _WIN32
#undef HW_WIN
#define HW_WIN 1
#define NOMINMAX

// Determining the architecture used
#if _WIN32 && !_WIN64
static_assert(sizeof(void*) == 4, "This code is for 32-bit systems.");
#define ARCH_64            0
#define ARCH_32         1
#else
static_assert(sizeof(void*) == 8, "This code is for 64-bit systems.");
#define ARCH_64            1
#define ARCH_32         0
#endif

// DLL export values

#endif


// For linux based systems
#ifdef __linux
#undef HW_LINUX
#define HW_LINUX 1
#endif
