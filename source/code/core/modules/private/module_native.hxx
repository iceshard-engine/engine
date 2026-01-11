/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os.hxx>
#include <ice/string.hxx>

namespace ice::native_module
{

#if ISP_WINDOWS

    using ModuleHandle = ice::win32::DynLibHandle;

#elif ISP_UNIX

    using ModuleHandle = ice::unix_::DynLibHandle;

#endif

    auto module_open(ice::String path) noexcept -> ice::native_module::ModuleHandle;

    void module_close(ice::native_module::ModuleHandle module) noexcept;

    auto module_find_address(ice::native_module::ModuleHandle const& module, ice::String symbol_name) noexcept -> void*;

} // namespace ice
