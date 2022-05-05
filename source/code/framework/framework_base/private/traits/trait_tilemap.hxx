#pragma once
#include <ice/asset_type.hxx>
#include <ice/game_physics.hxx>
#include <ice/game_tilemap.hxx>

#include <ice/pod/array.hxx>

namespace ice
{

    struct TileMapInstance
    {
        ice::TileMap const* tilemap;
        ice::PhysicsID* physics_ids;
    };

    class IceWorldTrait_TileMap : public ice::WorldTrait_TileMap
    {
    public:
        IceWorldTrait_TileMap(
            ice::Allocator& alloc,
            ice::WorldTrait_Physics2D& trait_physics
        ) noexcept;
        ~IceWorldTrait_TileMap() noexcept = default;

        void load_tilemap(
            ice::Utf8String tilemap
        ) noexcept override;

        auto load_tilemap_task(
            ice::Utf8String tilemap,
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>;

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
        ice::Allocator& _allocator;
        ice::WorldTrait_Physics2D& _physics;

        ice::Utf8String _requested_tilemap;
        ice::pod::Array<ice::TileMapInstance> _tilemaps;
    };

    class WorldTraitArchive;

    void register_trait_tilemap(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
