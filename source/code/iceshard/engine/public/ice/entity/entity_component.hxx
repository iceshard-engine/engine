#pragma once
#include <ice/stringid.hxx>

namespace ice
{

    enum class Entity : ice::u64;

    template<typename T>
    static constexpr bool IsEntityType = std::is_same_v<ice::Entity, T>;

    template<typename T>
    concept EntityComponent = std::is_trivially_copyable_v<ice::clean_type<T>>
        && requires(T x) {
        { ice::clean_type<T>::Identifier } -> std::convertible_to<ice::StringID const>;
    };

    template<typename T>
    concept EntityTag = EntityComponent<T> && std::is_empty_v<T>;

    template<typename T>
        requires EntityComponent<T> || IsEntityType<T>
    static constexpr ice::StringID ComponentIdentifier = T::Identifier;

    template<>
    static constexpr ice::StringID ComponentIdentifier<ice::Entity> = "ice.__entity__"_sid;

} // namespace ice
