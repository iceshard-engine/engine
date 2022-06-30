#pragma once
#include <ice/game_ui.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_entity_tracker.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/pod/hash.hxx>
#include <ice/ui_data.hxx>
#include <ice/asset.hxx>

namespace ice
{

    struct UIRuntimeElement
    {
        ice::ui::ElementInfo const* info;
        ice::vec2f position;
        ice::vec2f size;
    };

    struct UIRuntimePage
    {
        ice::u64 info_hash;
        ice::ecs::EntityHandle current_handle;
    };


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
        auto load_ui(
            ice::Allocator& alloc,
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::Utf8String name
        ) noexcept -> ice::Task<>;

        auto show_ui(
            ice::ecs::EntityHandle ui_entity
        ) noexcept -> ice::Task<>;

        auto hide_ui(
            ice::ecs::EntityHandle ui_entity
        ) noexcept -> ice::Task<>;

    private:
        struct PageInfo
        {
            ice::Utf8String name;
            ice::ui::UIData const* data;
            ice::Span<ice::ui::Element> elements;

            ice::AssetHandle* asset_handle;
        };

        struct Page
        {
            ice::u64 info_hash;
            ice::ecs::EntityHandle current_handle;
        };

        ice::Allocator& _allocator;
        ice::ecs::EntityTracker _entity_tracker;

        ice::pod::Hash<PageInfo> _pages_info;
        ice::pod::Hash<Page> _pages;

        ice::Utf8String _visible_page;

        ice::vec2f _size_fb;
        ice::vec2f _pos_mouse;
    };

    void register_trait_gameui(
        ice::WorldTraitArchive& trait_archive
    ) noexcept;

} // namespace ice
