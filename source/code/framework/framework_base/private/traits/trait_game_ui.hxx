#pragma once
#include <ice/game_ui.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/pod/hash.hxx>
#include <ice/ui_data.hxx>
#include <ice/asset.hxx>

namespace ice
{

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

            ice::AssetHandle* asset_handle;
        };

        struct Page
        {
            ice::u64 info_hash;
            ice::ecs::EntityHandle current_handle;
        };

        ice::Allocator& _allocator;
        ice::pod::Hash<PageInfo> _pages_info;
        ice::pod::Hash<Page> _pages;
    };

} // namespace ice
