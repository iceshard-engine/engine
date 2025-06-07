/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/framework_app.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_graph.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>
#include <ice/mem_allocator_proxy.hxx>

#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

using namespace ice;
using namespace ice::framework;

class TestGame : public Game
{
public:
    TestGame(ice::Allocator& alloc) noexcept;

    void on_setup(ice::framework::State const& state) noexcept override;
    void on_shutdown(ice::framework::State const& state) noexcept override;

    void on_resume(ice::Engine& engine) noexcept override;
    void on_update(ice::Engine& engine, ice::EngineFrame& frame) noexcept override;
    void on_suspend(ice::Engine& engine) noexcept override;

    auto rendergraph(ice::gfx::GfxContext& device) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime> override;

private:
    ice::Allocator& _allocator;
    ice::ProxyAllocator _tasks_alloc;

    ice::UniquePtr<ice::gfx::GfxGraph> _graph;
    ice::UniquePtr<ice::gfx::GfxGraphRuntime> _graph_runtime;

    ice::UniquePtr<ice::ecs::ArchetypeIndex> _archetype_index;
    ice::UniquePtr<ice::ecs::EntityStorage> _entity_storage;

    bool _first_time;
};
