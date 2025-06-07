/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include <ice/app_info.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>
#include <ice/tool_app.hxx>

auto hscp_process_directory(ice::Allocator& alloc, ice::String dir) noexcept -> ice::HeapString<>
{
    ice::HeapString<> searched_utf8_path{ alloc, dir };
    if (ice::path::is_absolute(dir) == false)
    {
        searched_utf8_path = ice::app::workingdir();
        ice::path::join(searched_utf8_path, dir);
    }

    ice::path::normalize(searched_utf8_path);
    return searched_utf8_path;
}
