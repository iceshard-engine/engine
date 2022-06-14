#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/engine_types.hxx>
#include <ice/ecs/ecs_types.hxx>

#include <ice/gfx/gfx_types.hxx>
#include <ice/input/input_types.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice
{

    struct WorldTemplate;

    class AssetStorage;
    class WorldTraitArchive;

    struct RenderQueueDefinition
    {
        ice::StringID name;
        ice::render::QueueFlags flags;
    };

    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        virtual auto create_runner(
            ice::UniquePtr<ice::input::InputTracker> input_tracker,
            ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
        ) noexcept -> ice::UniquePtr<ice::EngineRunner> = 0;

        virtual auto create_graphics_runner(
            ice::render::RenderDriver& render_driver,
            ice::render::RenderSurface& render_surface,
            ice::WorldTemplate const& render_world_template,
            ice::Span<ice::RenderQueueDefinition const> render_queues
        ) noexcept -> ice::UniquePtr<ice::gfx::GfxRunner> = 0;

        virtual void update_runner_graphics(
            ice::EngineRunner& runner,
            ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
        ) noexcept = 0;

        virtual auto entity_index() noexcept -> ice::ecs::EntityIndex& = 0;

        virtual auto asset_storage() noexcept -> ice::AssetStorage& = 0;

        virtual auto world_manager() noexcept -> ice::WorldManager& = 0;

        virtual auto world_trait_archive() const noexcept -> ice::WorldTraitArchive const& = 0;

        virtual auto developer_ui() noexcept -> ice::EngineDevUI& = 0;
    };

} // namespace ice
