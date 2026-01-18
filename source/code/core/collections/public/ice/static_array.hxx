/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>

namespace ice
{

    // TODO: Introduce our own type and create proper concepts for function access.
    template<typename T, ice::u32 Size>
    using StaticArray = std::array<T, Size>;

} // namespace ice
