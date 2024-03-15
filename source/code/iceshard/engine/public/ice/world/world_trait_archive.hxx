/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/world/world_trait_descriptor.hxx>
#include <ice/world/world_updater.hxx>

namespace ice
{

    struct TraitArchive
    {
        virtual ~TraitArchive() noexcept = default;

        virtual void register_trait(ice::TraitDescriptor trait_descriptor) noexcept = 0;
        virtual auto trait(ice::StringID_Arg traitid) const noexcept -> ice::TraitDescriptor const* = 0;
    };

    auto create_default_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::TraitArchive>;

} // namespace ice
