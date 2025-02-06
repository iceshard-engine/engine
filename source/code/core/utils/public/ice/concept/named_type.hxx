/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>

namespace ice::concepts
{

    template<typename T>
    concept NamedDataType = requires(T t) {
        { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
    } && std::is_trivially_destructible_v<T>;

} // namespace ice
