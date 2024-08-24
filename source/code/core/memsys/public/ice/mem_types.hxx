/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/concept/strong_type_integral.hxx>

namespace ice
{

    struct usize;
    struct isize;
    struct meminfo;

    enum class ualign : ice::u32;

    struct Data;
    struct Memory;

    template<bool WithDebugInfo>
    struct AllocatorBase;

    class AllocatorDebugInfo;

    using Allocator = ice::AllocatorBase<ice::build::is_debug || ice::build::is_develop>;

} // namespace ice
