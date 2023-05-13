/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <algorithm>

namespace ice::ecs
{

    namespace detail
    {

        template<typename T>
        concept HasIdentifierMember = requires(T x) {
            { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
        };

        template<typename T>
        constexpr auto constexpr_sort_array(T const& arr, ice::u32 start_offset = 0) noexcept
        {
            auto result = arr;
            std::sort(std::next(std::begin(result), start_offset), std::end(result));
            return result;
        }

    } // namespace detail

} // namespace ice::ecs
