/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_detail.hxx>

namespace ice::ecs
{

    template<typename T>
    concept Component = std::is_trivially_copyable_v<ice::clear_type_t<T>>
        && ice::ecs::detail::HasIdentifierMember<T>;

    template<typename T>
    concept ComponentTag = Component<T> && std::is_empty_v<T>;


    template<typename T> requires Component<T> || IsEntity<T>
    constexpr ice::StringID Constant_ComponentIdentifier = T::Identifier;

    template<>
    constexpr inline ice::StringID Constant_ComponentIdentifier<ice::ecs::Entity> = "ice.__ecs_entity__"_sid;


    template<typename T> requires Component<T> || IsEntity<T>
    constexpr ice::u32 Constant_ComponentSize = ComponentTag<T> ? 0 : sizeof(T);

    template<typename T> requires Component<T> || IsEntity<T>
    constexpr ice::u32 Constant_ComponentAlignment = ComponentTag<T> ? 0 : alignof(T);

} // namespace ice::ecs
