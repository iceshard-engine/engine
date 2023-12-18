/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/engine_devui.hxx>
#include <ice/engine_types.hxx>
#include <ice/world/world_trait_archive.hxx>

namespace ice::devui
{

    class DevUIWidget;

    class DevUISystem : public ice::EngineDevUI
    {
    public:
        virtual ~DevUISystem() noexcept = default;
    };

} // namespace ice::devui
