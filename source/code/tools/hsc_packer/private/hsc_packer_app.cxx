/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include <ice/app_info.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>
#include <ice/tool_app.hxx>

auto hscp_process_directory(ice::Allocator& alloc, ice::Path dir) noexcept -> ice::HeapPath
{
    ice::HeapPath searched_utf8_path{ alloc, dir };
    if (dir.is_relative())
    {
        searched_utf8_path = ice::app::workingdir();
        searched_utf8_path.join(dir);
    }

    searched_utf8_path.normalize();
    return searched_utf8_path;
}
