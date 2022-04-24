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
    };

    auto create_world_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTraitArchive>;

} // namespace ice
