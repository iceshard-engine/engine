#include "trait_game_ui.hxx"
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/shard_container.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread_pool.hxx>

#include <ice/pod/array.hxx>
#include <ice/asset_storage.hxx>
#include <ice/ui_asset.hxx>
#include <ice/game_ui.hxx>

namespace ice
{

    IceWorldTrait_GameUI::IceWorldTrait_GameUI(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _pages_info{ _allocator }
        , _pages{ _allocator }
    {
    }

    void IceWorldTrait_GameUI::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
    }

    void IceWorldTrait_GameUI::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
    }

    void IceWorldTrait_GameUI::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::pod::Array<ice::c8utf const*> page_names{ frame.allocator() };
        if (ice::shards::inspect_all<ice::c8utf const*>(runner.previous_frame().shards(), ice::Shard_GameUI_Load, page_names) > 0)
        {
            for (ice::c8utf const* page_name : page_names)
            {
                portal.execute(
                    load_ui(portal.allocator(), frame, runner, page_name)
                );
            }
        }



        ice::shards::inspect_each<ice::ecs::Entity>(
            runner.previous_frame().shards(),
            ice::Shard_GameUI_Show,
            [this](ice::ecs::Entity entity)
            {

            }
        );

    }

    auto IceWorldTrait_GameUI::load_ui(
        ice::Allocator& alloc,
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::Utf8String name
    ) noexcept -> ice::Task<>
    {
        ice::Asset page_asset = co_await runner.asset_storage().request(ice::ui::AssetType_UIPage, name, AssetState::Loaded);
        if (ice::asset_check(page_asset, AssetState::Loaded) == false)
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Game,
                "UI Page with name {} couldn't be loaded.",
                ice::String{ (char const*)name.data(), name.size() }
            );
            co_return;
        }

        co_await runner.schedule_next_frame();

        ice::ui::UIData const* page_data = reinterpret_cast<ice::ui::UIData const*>(page_asset.data.location);

        PageInfo const page_info{
            .name = name,
            .data = page_data,
            .asset_handle = page_asset.handle,
        };

        ice::pod::hash::set(
            _pages_info,
            ice::hash(page_info.name),
            page_info
        );
    }

} // namespace ice
