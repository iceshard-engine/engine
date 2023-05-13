/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_manager.hxx>
#include "iceshard_world.hxx"

namespace ice
{

    class IceshardWorldManager final : public ice::WorldManager
    {
    public:
        IceshardWorldManager(
            ice::Allocator& alloc,
            ice::WorldTraitArchive const& trait_archive
        ) noexcept;
        ~IceshardWorldManager() noexcept override;

        auto create_world(
            ice::Allocator& alloc,
            ice::WorldTemplate const& world_template
        ) const noexcept -> ice::World* override;

        auto create_world(
            ice::WorldTemplate const& world_template
        ) noexcept -> World* override;

        auto find_world(
            ice::StringID_Arg name
        ) noexcept -> World* override;

        void destroy_world(
            ice::StringID_Arg name
        ) noexcept override;

        auto worlds() const noexcept -> ice::Span<ice::IceshardWorld* const>;

    private:
        ice::Allocator& _allocator;
        ice::WorldTraitArchive const& _trait_archive;
        ice::HashMap<ice::IceshardWorld*> _worlds;
    };

} // namespace ice
