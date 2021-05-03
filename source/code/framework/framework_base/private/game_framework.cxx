#include <ice/game_framework.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/gfx/gfx_device.hxx>

#include <ice/render/render_module.hxx>
#include <ice/render/render_surface.hxx>

#include <ice/input/input_tracker.hxx>

#include <ice/platform_window_surface.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class GameFrameworkApp final : public ice::platform::App
    {
    public:
        GameFrameworkApp(
            ice::GameFramework& framework,
            ice::SystemClock& clock,
            ice::UniquePtr<ice::platform::WindowSurface> window_surface,
            ice::UniquePtr<ice::render::RenderDriver> render_driver,
            ice::render::RenderSurface* render_surface, // #todo
            ice::UniquePtr<ice::EngineRunner> runner
        ) noexcept
            : _framework{ framework }
            , _clock{ clock }
            , _window_surface{ ice::move(window_surface) }
            , _render_driver{ ice::move(render_driver) }
            , _render_surface{ render_surface }
            , _runner{ ice::move(runner) }
        {
            _framework.game_begin(*_runner);
        }

        ~GameFrameworkApp() noexcept override
        {
            _runner = nullptr;
            _framework.game_end();
            _render_driver->destroy_surface(_render_surface);
            _render_driver = nullptr;
            _window_surface = nullptr;
        }

        void handle_inputs(
            ice::input::DeviceQueue const& device_events
        ) noexcept override
        {
            ice::clock::update(_clock);
            _runner->process_device_queue(device_events);
        }

        void update(
            ice::pod::Array<ice::platform::Event> const& events
        ) noexcept override
        {
            _runner->next_frame();
        }

    private:
        ice::GameFramework& _framework;
        ice::SystemClock& _clock;
        ice::UniquePtr<ice::platform::WindowSurface> _window_surface;
        ice::UniquePtr<ice::render::RenderDriver> _render_driver;
        ice::render::RenderSurface* _render_surface;
        ice::UniquePtr<ice::EngineRunner> _runner;
    };

    GameFramework::GameFramework(
        ice::Allocator& alloc,
        ice::ResourceSystem& resource_system,
        ice::ModuleRegister& module_register
    ) noexcept
        : _system_clock{ ice::clock::create_clock() }
        , _allocator{ alloc }
        , _resource_system{ resource_system }
        , _module_register{ module_register }
        , _current_engine{ nullptr }
    {
    }

    GameFramework::~GameFramework() noexcept
    {
    }

    auto GameFramework::resource_system() noexcept -> ice::ResourceSystem&
    {
        return _resource_system;
    }

    auto GameFramework::module_registry() noexcept -> ice::ModuleRegister&
    {
        return _module_register;
    }

    auto GameFramework::platform_app() noexcept -> ice::UniquePtr<ice::platform::App>
    {
        ice::UniquePtr<ice::platform::App> app_obj = ice::make_unique_null<platform::App>();

        // Create input tracker
        ice::UniquePtr<ice::input::InputTracker> input_tracker = ice::input::create_default_input_tracker(_allocator, _system_clock);
        if (input_tracker == nullptr)
        {
            return app_obj;
        }

        input_tracker->register_device_type(
            ice::input::DeviceType::Keyboard,
            ice::input::get_default_device_factory()
        );
        input_tracker->register_device_type(
            ice::input::DeviceType::Mouse,
            ice::input::get_default_device_factory()
        );

        // Create app surface
        ice::UniquePtr<ice::platform::WindowSurface> app_surface = ice::platform::create_window_surface(
            _allocator, ice::render::RenderDriverAPI::Vulkan
        );
        if (app_surface == nullptr)
        {
            return app_obj;
        }

        // Load render driver
        ice::UniquePtr<ice::render::RenderDriver> render_driver = ice::render::create_render_driver(
            _allocator, _module_register
        );
        if (render_driver == nullptr)
        {
            return app_obj;
        }

        // Query surface details and create render surface
        ice::render::SurfaceInfo surface_info;

        [[maybe_unused]]
        bool const query_success = app_surface->query_details(surface_info);
        ICE_ASSERT(query_success, "Couldn't query surface details from platform surface!");

        ice::render::RenderSurface* render_surface = render_driver->create_surface(surface_info);
        if (render_surface == nullptr)
        {
            return app_obj;
        }

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
            .render_driver = render_driver.get(),
            .render_surface = render_surface,
            .queue_list = gfx_queue_info
        };

        ice::UniquePtr<ice::EngineRunner> runner = _current_engine->create_runner(
            ice::move(input_tracker),
            gfx_device_info
        );
        if (runner == nullptr)
        {
            render_driver->destroy_surface(render_surface);
            return app_obj;
        }

        return ice::make_unique<platform::App, GameFrameworkApp>(
            _allocator,
            *this,
            _system_clock,
            ice::move(app_surface),
            ice::move(render_driver),
            render_surface,
            ice::move(runner)
        );
    }

    void GameFramework::startup(ice::Engine& engine) noexcept
    {
        _current_engine = &engine;
        this->on_app_startup_internal(engine);
    }

    void GameFramework::shutdown(ice::Engine& engine) noexcept
    {
        this->on_app_shutdown_internal(engine);
        _current_engine = nullptr;
    }

    void GameFramework::game_begin(ice::EngineRunner& runner) noexcept
    {
        this->on_game_begin_internal(runner);
    }

    void GameFramework::game_end() noexcept
    {
        this->on_game_end_internal();
    }

} // namespace ice