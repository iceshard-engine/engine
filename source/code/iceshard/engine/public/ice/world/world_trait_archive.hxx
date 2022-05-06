#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait_description.hxx>

namespace ice
{

    class WorldTraitArchive
    {
    public:
        virtual ~WorldTraitArchive() noexcept = default;

        virtual void register_trait(
            ice::StringID_Arg name,
            ice::WorldTraitDescription description
        ) noexcept = 0;

        virtual auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTraitDescription const* = 0;

        virtual bool validate_trait_list(
            ice::Span<ice::StringID const> traits
        ) const noexcept = 0;
    };

    auto create_world_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTraitArchive>;

    template<typename TraitType>
    auto register_trait_default(
        ice::WorldTraitArchive& archive,
        ice::StringID_Arg name
    ) noexcept
    {
        archive.register_trait(
            name,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<TraitType>
            }
        );
    }

} // namespace ice
