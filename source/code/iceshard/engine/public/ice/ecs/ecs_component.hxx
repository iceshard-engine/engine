#pragma once
#include <ice/stringid.hxx>
#include <ice/ecs/ecs_entity.hxx>

namespace ice::ecs
{

    // #todo move to a different file with implementation details / utility.
    template<typename T>
    concept HasIdentifierMember = requires(T x) {
        { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
    };


    template<typename T>
    concept Component = std::is_trivially_copyable_v<ice::clear_type_t<T>>
        && HasIdentifierMember<T>;

    template<typename T>
    concept ComponentTag = Component<T> && std::is_empty_v<T>;


    // #todo move to a different file with implementation details / utility.
    template<typename T> requires Component<T> || IsEntityHandle<T>
    static constexpr ice::StringID ComponentIdentifier = T::Identifier;

    // #todo move to a different file with implementation details / utility.
    template<>
    static constexpr ice::StringID ComponentIdentifier<ice::ecs::EntityHandle> = "ice.__ecs_entity_handle__"_sid;

} // namespace ice::ecs
