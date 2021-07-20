#pragma once
#include <ice/entity/entity_component.hxx>
#include <ice/pod/array.hxx>
#include <algorithm>
#include <array>

namespace ice
{

    enum class ArchetypeHandle : ice::u32
    {
        Invalid = 0x0
    };

    struct ArchetypeComponent
    {
        ice::StringID_Hash name;
        ice::u32 size;
        ice::u32 alignment;
    };

    template<EntityComponent... Components>
    struct Archetype
    {
        constexpr Archetype() noexcept;

        std::array<ArchetypeComponent, 1 + sizeof...(Components)> components;
    };

    constexpr auto archetype_handle(
        ice::Span<ice::ArchetypeComponent const> components
    ) noexcept -> ice::ArchetypeHandle
    {
        ice::u32 handle_hash = ice::hash32("ice.__archetype__");
        for (ArchetypeComponent const& component : components)
        {
            handle_hash <<= 5;
            handle_hash ^= ice::hash32(ice::hash(component.name) >> 17);
        }
        return static_cast<ArchetypeHandle>(handle_hash);
    }

    template<EntityComponent... Components>
    constexpr auto archetype_handle(
        ice::Archetype<Components...> archetype = {}
    ) noexcept -> ice::ArchetypeHandle
    {
        return archetype_handle(archetype.components);
    }

    constexpr bool operator==(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept;

    constexpr bool operator!=(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept;

    constexpr bool operator<(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept;

    namespace detail
    {

        template<typename... Components>
        struct ArchetypeUnsorted
        {
            static constexpr std::array<ArchetypeComponent, sizeof...(Components)> Const_UnsortedCcomponents{
                ArchetypeComponent{
                    .name = ice::stringid_hash(ice::ComponentIdentifier<Components>),
                    .size = sizeof(Components),
                    .alignment = alignof(Components),
                }...
            };
        };

        inline void sort_component_array(
            ice::pod::Array<ice::StringID>& arr
        ) noexcept
        {
            static const auto pred = [](ice::StringID_Arg left, ice::StringID_Arg right)
            {
                return ice::hash(left) < ice::hash(right);
            };

            std::sort(ice::pod::begin(arr) + 1, ice::pod::end(arr), pred);
        }

        template<typename T>
        constexpr auto sort_component_array(T const& arr) noexcept
        {
            auto result = arr;
            std::sort(std::begin(result) + 1, std::end(result));
            return result;
        }

    } // namespace detail

    template<EntityComponent... Components>
    constexpr Archetype<Components...>::Archetype() noexcept
        : components{
            detail::sort_component_array(
                detail::ArchetypeUnsorted<ice::Entity, Components...>::Const_UnsortedCcomponents
            )
        }
    { }

    constexpr bool operator==(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept
    {
        return ice::hash(left.name) == ice::hash(right.name)
            && left.size == right.size
            && left.alignment == right.alignment;
    }

    constexpr bool operator!=(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept
    {
        return !(left == right);
    }

    constexpr bool operator<(
        ArchetypeComponent const& left,
        ArchetypeComponent const& right
    ) noexcept
    {
        return ice::hash(left.name) < ice::hash(right.name);
    }

    namespace detail
    {

        struct ValidationComponent_Position2D
        {
            static constexpr ice::StringID Identifier = "ice.position2d"_sid;

            ice::vec2f pos;
        };

        struct ValidationComponent_Velocity2D
        {
            static constexpr ice::StringID Identifier = "ice.velocity2d"_sid;

            ice::vec2f vel;
            ice::vec2f angVel;
        };

        auto constexpr validation_physic2d_archetype = Archetype<ValidationComponent_Position2D, ValidationComponent_Velocity2D>{ };
        auto constexpr validation_physic2d_archetype_2 = Archetype<ValidationComponent_Velocity2D, ValidationComponent_Position2D>{ };

        static_assert(
            Archetype<ValidationComponent_Position2D, ValidationComponent_Velocity2D>{ }.components[0]
            ==
            Archetype<ValidationComponent_Velocity2D, ValidationComponent_Position2D>{ }.components[0]
        );

        static_assert(validation_physic2d_archetype.components.size() == 3);
        static_assert(validation_physic2d_archetype_2.components.size() == 3);

        static_assert(validation_physic2d_archetype_2.components[0].name == validation_physic2d_archetype.components[0].name);
        static_assert(validation_physic2d_archetype_2.components[1].name == validation_physic2d_archetype.components[1].name);

        static_assert(validation_physic2d_archetype_2.components[0].name == validation_physic2d_archetype.components[0].name);
        static_assert(validation_physic2d_archetype_2.components[1].name == validation_physic2d_archetype.components[1].name);

    } // namespace detail

} // namespace ice
