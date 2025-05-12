/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_file.hxx>

namespace ice::tool
{

    auto global_allocator() noexcept -> ice::Allocator&;

    auto path_current_directory() noexcept -> ice::native_file::HeapFilePath;

    auto path_make_absolute(ice::native_file::FilePath path) noexcept -> ice::native_file::HeapFilePath;

} // namespace ice
