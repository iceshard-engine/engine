/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string/string.hxx>
#include <ice/span.hxx>
#include <type_traits>

namespace ice
{

    struct ParamsInternal;
    struct ParamInstanceBase;

    using Params = ice::UniquePtr<ParamsInternal> const;
    using ParamsCustomCallback = bool(*)(void* userdata, ice::Span<ice::String const> results) noexcept;

    namespace concepts
    {

        template<typename T>
        concept ParamCustomType = requires(T t) {
            std::is_same_v<decltype(&T::param_parse_results), bool(*)(T&, ice::Span<ice::String const>) noexcept>;
        };

    } // namespace concepts

} // namespace ice
