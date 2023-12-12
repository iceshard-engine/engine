/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_file.hxx>

namespace ice::tool
{

    auto global_allocator() noexcept -> ice::Allocator&;

    auto working_directory() noexcept -> ice::native_file::HeapFilePath;

} // namespace ice
