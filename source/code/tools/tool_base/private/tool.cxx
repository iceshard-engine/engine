/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/log.hxx>
#include <ice/log_module.hxx>
#include <ice/module_register.hxx>
#include <ice/tool_app.hxx>

auto ice::tool::global_allocator() noexcept -> ice::Allocator&
{
    static ice::HostAllocator alloc;
    return alloc;
}

auto ice::tool::working_directory() noexcept -> ice::native_file::HeapFilePath
{
    ice::u32 const dirsize = GetCurrentDirectoryW(0, 0) - 1; // We don't need the '0' null count
    ice::native_file::HeapFilePath current_workingdir{ global_allocator() };
    ice::string::resize(current_workingdir, dirsize);
    GetCurrentDirectoryW(current_workingdir._capacity, current_workingdir._data);
    ice::string::push_back(current_workingdir, L'\\');
    ice::path::normalize(current_workingdir);
    return current_workingdir;
}
