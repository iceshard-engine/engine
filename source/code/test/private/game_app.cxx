#include "game_app.hxx"
#include <ice/stringid.hxx>
#include <ice/uri.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/engine.hxx>

#include <ice/log.hxx>
#include <ice/assert.hxx>

using ice::operator""_sid;
using ice::operator""_uri;

TestGameApp::TestGameApp(
    ice::Allocator& alloc,
    ice::Engine& engine,
    ice::render::RenderDriver& render_driver
) noexcept
    : _allocator{ alloc }
    , _engine{ engine }
    , _render_driver{ render_driver }
    , _platform_surface{ ice::make_unique_null<ice::platform::WindowSurface>() }
    , _render_surface{ nullptr }
    , _game{ ice::make_unique_null<TestGame>() }
{
    _platform_surface = ice::platform::create_window_surface(
        alloc, ice::render::RenderDriverAPI::Vulkan
    );

    ice::render::SurfaceInfo surface_info;

    [[maybe_unused]]
    bool const query_success = _platform_surface->query_details(surface_info);
    ICE_ASSERT(query_success, "Couldn't query surface details from platform surface!");

    _render_surface = _render_driver.create_surface(surface_info);

    ice::gfx::GfxQueueCreateInfo gfx_queue_info[]{
        ice::gfx::GfxQueueCreateInfo{
            .name = "default"_sid,
            .queue_flags = ice::render::QueueFlags::Graphics | ice::render::QueueFlags::Present
        },
        ice::gfx::GfxQueueCreateInfo{
            .name = "transfer"_sid,
            .queue_flags = ice::render::QueueFlags::Transfer
        }
    };

    ice::gfx::GfxDeviceCreateInfo gfx_device_info{
        .render_driver = &_render_driver,
        .render_surface = _render_surface,
        .queue_list = gfx_queue_info
    };

    _game = ice::make_unique<TestGame>(
        _allocator,
        _allocator,
        _engine,
        _engine.create_runner(gfx_device_info)
    );
}

TestGameApp::~TestGameApp() noexcept
{
    _game = nullptr;
    _render_driver.destroy_surface(_render_surface);
    _platform_surface = nullptr;
}

void TestGameApp::handle_inputs(
    ice::input::DeviceQueue const& device_events
) noexcept
{

}

void TestGameApp::update(
    ice::pod::Array<ice::platform::Event> const& events
) noexcept
{
    _game->update();
}
