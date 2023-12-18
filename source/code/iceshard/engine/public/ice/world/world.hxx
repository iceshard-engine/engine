/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/data_storage.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/engine_types.hxx>
#include <ice/span.hxx>

namespace ice
{

    enum class WorldState : ice::u8
    {
        Idle,
        Active,
        Disabled,
    };

    struct WorldDefinition
    {
        ice::Span<ice::StringID> traits;
    };

    struct World
    {
        virtual ~World() noexcept = default;

        virtual auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* = 0;
        virtual auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* = 0;

        virtual auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* = 0;
        virtual auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* = 0;
    };

} // namespace ice
