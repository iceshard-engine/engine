#pragma once
#include <ice/base.hxx>

#if ISP_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace ice::win32
{

    using ModuleHandle = HMODULE;

} // namespace ice::win32

#endif // ISP_WINDOWS
