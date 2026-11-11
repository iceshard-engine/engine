/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
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

    // MEMORY TYPE CONCEPTS

    namespace concepts
    {

        template<typename T>
        concept RWDataObject = requires(T t) {
            { t.location } -> std::convertible_to<void*>;
            { t.size } -> std::convertible_to<ice::usize>;
            { t.alignment } -> std::convertible_to<ice::ualign>;
        };

        template<typename T>
        concept RODataObject = requires(T t) {
            { t.location } -> std::convertible_to<void const*>;
            { t.size } -> std::convertible_to<ice::usize>;
            { t.alignment } -> std::convertible_to<ice::ualign>;
        };

        template<typename T>
        concept PointerType = std::is_pointer_v<T>;

        template<typename T>
        concept SizeType = std::is_same_v<T, ice::usize> || std::is_same_v<T, ice::isize>;

        template<typename T>
        concept VoidPointer = std::is_same_v<T, void const*> || std::is_same_v<T, void*>;

    } // namespace concepts

    template<ice::concepts::PointerType T>
    using as_void_ptr_t = ice::const_correct_t<std::remove_pointer_t<T>, void>*;

} // namespace ice
