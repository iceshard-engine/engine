/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include <ice/os/windows.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>
#include <ice/tool_app.hxx>

auto hscp_process_directory(ice::Allocator& alloc, ice::String dir) noexcept -> ice::HeapString<>
{
    ice::HeapString<> searched_utf8_path{ alloc, dir };
    if (ice::path::is_absolute(dir) == false)
    {
        ice::string::clear(searched_utf8_path);
        ice::wide_to_utf8_append(ice::tool::path_current_directory(), searched_utf8_path);
        ice::string::push_back(searched_utf8_path, dir);
    }

    ice::path::normalize(searched_utf8_path);
    return searched_utf8_path;
}
