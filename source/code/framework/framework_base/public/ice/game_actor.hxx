/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/input/input_types.hxx>

namespace ice
{

    static constexpr ice::StringID Constant_TraitName_Actor
        = "ice.base-framework.trait-actor"_sid;

    enum class ActorType : ice::u32
    {
        Player,
    };

    struct Actor
    {
        static constexpr ice::StringID Identifier = "ice.component.actor"_sid;

        ice::ActorType type;
    };

} // namespace ice
