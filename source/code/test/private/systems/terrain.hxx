#pragma once
#include <ice/engine.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/unique_ptr.hxx>

namespace ice::trait
{

    class Terrain final : public ice::WorldTrait
    {
    public:
        Terrain(
            ice::Allocator& alloc,
            ice::Engine& engine
        ) noexcept;
        ~Terrain() noexcept override = default;

        void on_activate(
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;

        void on_deactivate(
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;

        struct RenderCache;

    private:
        ice::Engine& _engine;
        ice::AssetSystem& _asset_system;
        ice::UniquePtr<RenderCache> _render_cache;
    };

} // namespace ice::trait
