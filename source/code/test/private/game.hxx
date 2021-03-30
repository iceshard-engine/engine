#pragma once
#include <ice/allocator.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/world/world.hxx>
#include <ice/engine.hxx>

class TestGame final
{
public:
    TestGame(
        ice::Allocator& alloc,
        ice::Engine& engine,
        ice::UniquePtr<ice::EngineRunner> runner
    ) noexcept;
    ~TestGame() noexcept;

    void update() noexcept;

private:
    ice::Allocator& _allocator;
    ice::Engine& _engine;
    ice::UniquePtr<ice::EngineRunner> _runner;
    ice::gfx::GfxDevice& _gfx_device;
    ice::render::RenderDevice& _render_device;

    ice::UniquePtr<ice::gfx::GfxPass> _gfx_pass;

    ice::UniquePtr<ice::ArchetypeIndex> _archetype_index;
    ice::UniquePtr<ice::ArchetypeBlockAllocator> _archetype_alloc;
    ice::UniquePtr<ice::EntityStorage> _entity_storage;
    ice::World* _world;

    ice::pod::Array<ice::gfx::GfxStage*> _stages;
};

