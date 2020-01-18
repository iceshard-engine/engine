#include <core/base.hxx>

//!
//! This file should never be included as a general header to not spill all windows definitions into the global namespace.
//!

#if ISP_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#endif
