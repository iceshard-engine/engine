/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool.hxx>
#include <ice/app_info.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/string/heap_string.hxx>
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
    ICE_ASSERT_CORE(false); // Test the implementation!
    ice::native_file::HeapFilePath current_workingdir{ global_allocator() };
    ice::native_file::path_from_string(current_workingdir, ice::app::workingdir());
    return current_workingdir;
}

auto ice::tool::path_make_absolute(ice::native_file::FilePath path) noexcept -> ice::native_file::HeapFilePath
{
    ice::native_file::HeapFilePath searched_utf8_path{ global_allocator(), path };
    if (ice::path::is_absolute(path) == false)
    {
        ice::string::clear(searched_utf8_path);
        ice::path::join(searched_utf8_path, ice::tool::path_current_directory());
        ice::path::join(searched_utf8_path, path);
    }

    ice::path::normalize(searched_utf8_path);
    return searched_utf8_path;
}
