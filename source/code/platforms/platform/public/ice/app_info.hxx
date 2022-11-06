/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>

namespace ice
{

    void app_location(ice::HeapString<>& out) noexcept;
    void working_directory(ice::HeapString<>& out) noexcept;

} // namespace ice
