/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/string_types.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/engine_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/module_register.hxx>
#include <ice/gfx/gfx_graph_runtime.hxx>

namespace ice::framework
{

    // TODO: Move later into a separate file.
    static constexpr ice::ErrorCode E_FailedLoadingIceshardLibrary{ "E.2990:Framework:Failed to load iceshard.dll library!" };
    static constexpr ice::ErrorCode E_FailedCreateEngineObject{ "E.2991:Framework:Failed to create Engine object" };
    static constexpr ice::ErrorCode E_FailedCreateRenderObject{ "E.2992:Framework:Failed to create Renderer object" };

    struct Config
    {
        ice::String module_dir;
        ice::Span<ice::String const> resource_dirs;
    };

    struct State
    {
        ice::ModuleRegister& modules;
        ice::ResourceTracker& resources;
        ice::ecs::ArchetypeIndex& archetypes;
    };

    class Game
    {
    public:
        Game() noexcept = default;
        virtual ~Game() noexcept = default;

        virtual void on_config(ice::framework::Config& config) noexcept { }
        virtual void on_setup(ice::framework::State const& state) noexcept = 0;
        virtual void on_shutdown(ice::framework::State const& state) noexcept = 0;

        virtual void on_resume(ice::Engine& engine) noexcept { }
        virtual void on_update(ice::Engine& engine, ice::EngineFrame& frame) noexcept { }
        virtual void on_suspend(ice::Engine& engine) noexcept { }

        virtual auto rendergraph(ice::gfx::GfxContext& device) noexcept -> ice::UniquePtr<ice::gfx::GfxGraphRuntime> { return {}; }
    };

    auto create_game(ice::Allocator& alloc) noexcept -> ice::UniquePtr<Game>;

} // namespace ice::framework
