/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool.hxx>
#include <ice/app_info.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/heap_string.hxx>
#include <ice/path_utils.hxx>
#include <ice/log_module.hxx>
#include <ice/log.hxx>

auto ice::tool::global_allocator() noexcept -> ice::Allocator&
{
    static ice::HostAllocator alloc;
    return alloc;
}

auto ice::tool::path_current_directory() noexcept -> ice::native_file::HeapFilePath
{
    ice::native_file::HeapFilePath current_workingdir{ global_allocator() };
    ice::native_file::path_from_string(current_workingdir, ice::app::workingdir());
    return current_workingdir;
}

auto ice::tool::path_make_absolute(ice::native_file::FilePath path) noexcept -> ice::native_file::HeapFilePath
{
    ice::native_file::HeapFilePath searched_utf8_path{ global_allocator(), path };
    if (path.is_relative())
    {
        searched_utf8_path.clear();
        searched_utf8_path.join(ice::tool::path_current_directory());
        searched_utf8_path.join(path);
    }

    searched_utf8_path.normalize();
    return searched_utf8_path;
}
