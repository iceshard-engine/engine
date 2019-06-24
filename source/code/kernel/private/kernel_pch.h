#pragma once

// Disabled warnings
#pragma warning(disable : 4455)

#include <memory>
#include <stdio.h>

#include <kernel/platform_defs.h>

#include <kernel/compiletime/stringid.h>

#include <kernel/types.h>
#include <kernel/kernel.h>

// Debug headers
#include <kernel/logger/logger.h>
#include <kernel/crash.h>


#if HW_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif