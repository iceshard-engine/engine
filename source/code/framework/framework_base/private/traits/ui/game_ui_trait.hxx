/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_ui.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_entity_tracker.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/ui_element_draw.hxx>
#include <ice/asset.hxx>
#include "render_ui_trait.hxx"

namespace ice
{

    class GameUI_Page;

    class IceWorldTrait_GameUI final : public ice::WorldTrait
    {
    public:
        IceWorldTrait_GameUI(
            ice::Allocator& alloc
        ) noexcept;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        struct PageInfo;

        auto load_ui(
            ice::Allocator& alloc,
            ice::EngineRunner& runner,
            ice::String name
        ) noexcept -> ice::Task<>;

        auto update_ui(
            ice::Allocator& alloc,
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            PageInfo const& info
        ) noexcept -> ice::Task<>;

        auto show_ui(
            ice::ecs::EntityHandle ui_entity
        ) noexcept -> ice::Task<>;

        auto hide_ui(
            ice::ecs::EntityHandle ui_entity
        ) noexcept -> ice::Task<>;

    private:

        ice::Allocator& _allocator;
        ice::ecs::EntityTracker _entity_tracker;

        ice::HashMap<GameUI_Page*> _pages;

        ice::String _visible_page;

        ice::vec2u _swapchain_size;
        ice::vec2f _pos_mouse;
    };

    void register_trait_gameui(
        ice::WorldTraitArchive& trait_archive
    ) noexcept;

} // namespace ice
