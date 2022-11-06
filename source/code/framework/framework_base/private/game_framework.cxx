/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/game_framework.hxx>
#include <ice/game_render_traits.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/world/world_assembly.hxx>

#include <ice/input/input_tracker.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class GameFrameworkApp final : public ice::platform::App
    {
    public:
        GameFrameworkApp(
            ice::GameFramework& framework,
            ice::SystemClock& clock,
            ice::UniquePtr<ice::EngineRunner> runner
        ) noexcept
            : _framework{ framework }
            , _clock{ clock }
            , _runner{ ice::move(runner) }
        {
            _framework.game_begin(*_runner);
        }

        ~GameFrameworkApp() noexcept override
        {
            _runner = nullptr;
            _framework.game_end();
        }

        void handle_inputs(
            ice::input::DeviceEventQueue const& device_events
        ) noexcept override
        {
            ice::clock::update(_clock);
            _runner->process_device_queue(device_events);
        }

        void update(ice::ShardContainer const& shards) noexcept override
        {
            _runner->next_frame(shards);
        }

        bool requested_exit() const noexcept override
        {
            return ice::shards::contains(_runner->previous_frame().shards(), "action/game/exit"_shard);
        }

    private:
        ice::GameFramework& _framework;
        ice::SystemClock& _clock;
        ice::UniquePtr<ice::EngineRunner> _runner;
    };

    GameFramework::GameFramework(
        ice::Allocator& alloc,
        ice::ResourceTracker& resource_system,
        ice::ModuleRegister& module_register
    ) noexcept
        : _allocator{ alloc }
        , _system_clock{ ice::clock::create_clock() }
        , _resource_system{ resource_system }
        , _module_register{ module_register }
        , _current_engine{ nullptr }
    {
    }

    GameFramework::~GameFramework() noexcept
    {
    }

    auto GameFramework::resource_system() noexcept -> ice::ResourceTracker&
    {
        return _resource_system;
    }

    auto GameFramework::module_registry() noexcept -> ice::ModuleRegister&
    {
        return _module_register;
    }

    auto GameFramework::create_app(
        ice::UniquePtr<ice::gfx::GfxRunner> gfx_runner
    ) noexcept -> ice::UniquePtr<ice::platform::App>
    {
        ice::UniquePtr<ice::platform::App> app_obj{ };

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

        ice::UniquePtr<ice::EngineRunner> runner = _current_engine->create_runner(
            ice::move(input_tracker),
            ice::move(gfx_runner)
        );
        if (runner == nullptr)
        {
            return app_obj;
        }

        return ice::make_unique<GameFrameworkApp>(
            _allocator,
            *this,
            _system_clock,
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
