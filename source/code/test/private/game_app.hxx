#pragma once
#include <ice/platform_app.hxx>
#include <ice/platform_window_surface.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/engine.hxx>
#include "game.hxx"

#include <ice/task.hxx>
#include <ice/task_thread.hxx>

class TestGameApp final : public ice::platform::App
{
public:
    TestGameApp(
        ice::Allocator& alloc,
        ice::Engine& engine,
        ice::render::RenderDriver& render_driver
    ) noexcept;
    ~TestGameApp() noexcept;

    void handle_inputs(
        ice::input::DeviceQueue const& device_events
    ) noexcept override;

    void update(
        ice::pod::Array<ice::platform::Event> const& events
    ) noexcept override;

private:
    ice::UniquePtr<ice::TaskThread> _task_thread;

    ice::SystemClock _app_clock;
    ice::Allocator& _allocator;
    ice::Engine& _engine;
    ice::render::RenderDriver& _render_driver;

    ice::UniquePtr<ice::platform::WindowSurface> _platform_surface;
    ice::render::RenderSurface* _render_surface;

    ice::UniquePtr<TestGame> _game;
};
