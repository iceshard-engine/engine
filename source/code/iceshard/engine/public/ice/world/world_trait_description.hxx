#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/allocator.hxx>

namespace ice
{

    class World;
    class WorldTrait;
    class WorldTraitTracker;

    using WorldTraitFactory = auto(ice::Allocator&, ice::WorldTraitTracker const&) noexcept -> ice::WorldTrait*;

    struct WorldTraitDescription
    {
        ice::WorldTraitFactory* factory;
        ice::Span<ice::StringID const> required_dependencies;
        ice::Span<ice::StringID const> optional_dependencies;
    };

    class WorldTraitTracker
    {
    public:
        virtual ~WorldTraitTracker() noexcept = default;

        virtual auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTrait* = 0;
    };

    namespace detail
    {

        template<typename TraitType>
        inline auto generic_trait_factory(ice::Allocator& alloc, ice::WorldTraitTracker const&) noexcept -> ice::WorldTrait*
        {
            static_assert(
                std::is_constructible_v<TraitType> ^ std::is_constructible_v<TraitType, ice::Allocator&>,
                "A generic trait type may only have a default constructor or take a single reference to an Allocator& object!"
            );

            if constexpr (std::is_constructible_v<TraitType>)
            {
                return alloc.make<TraitType>();
            }
            else
            {
                return alloc.make<TraitType>(alloc);
            }
        };

    } // namespace detail

} // namespace ice
