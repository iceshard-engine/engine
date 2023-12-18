#pragma once
#include <ice/span.hxx>
#include <ice/string_types.hxx>
#include <ice/engine_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/module_register.hxx>

namespace ice::framework
{

    struct Config
    {
        ice::String module_dir;
        ice::Span<ice::String const> resource_dirs;
    };

    struct State
    {
        ice::ModuleRegister& modules;
        ice::ResourceTracker& resources;
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

        virtual auto rendergraph(ice::gfx::GfxDevice& device) noexcept -> ice::UniquePtr<ice::gfx::v3::GfxGraphRuntime> { return {}; }
    };

    auto create_game(ice::Allocator& alloc) noexcept -> ice::UniquePtr<Game>;

} // namespace ice::framework
