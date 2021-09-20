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

    template<typename T> requires Component<T> || IsEntityHandle<T>
    static constexpr ice::StringID ComponentIdentifier = T::Identifier;

    template<>
    static constexpr ice::StringID ComponentIdentifier<ice::ecs::EntityHandle> = "ice.__ecs_entity_handle__"_sid;

} // namespace ice::ecs
