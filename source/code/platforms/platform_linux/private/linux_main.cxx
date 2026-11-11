/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/params.hxx>
#include <ice/log.hxx>
#include <ice/platform.hxx>
#include <ice/static_string.hxx>

#include "linux_sdl2_platform_render_surface.hxx"

int main(int argc, char const** argv)
{
    ice::HostAllocator host_alloc{ };

    // Define all platform-specific options
    ice::Params params = ice::create_params(host_alloc, "iceshard", "0.0.1", "IceShard engine parameters");
    ice::params_register_globals(params);
    ice::platform::linux::sdl2::UnixWindow_X11WaylandSDL2::define_options(params);

    // Define program-specific options
    ice::UniquePtr<ice::app::Config> config;
    if (ice_configure(host_alloc, params, config); config == nullptr)
    {
        ICE_LOG(ice::LogSeverity::Error, ice::LogTag::Core, "{}", "Failed to initialize config");
        return 1;
    }

    ice::Result const res = ice::params_process(params, argc, argv);
    if (res == ice::E_Error || res == ice::S_CommandLineHelpRequested)
    {
        ice::platform::shutdown();
        config.reset();
        params.reset();
        return 1;
    }

    ice::app::Factories app_factories{ };
    ice_init(host_alloc, app_factories);

    // Initialize window subsystem
    ice::platform::linux::sdl2::UnixWindow_X11WaylandSDL2::initialize_subsystem();

    ice::UniquePtr<ice::app::State> state = app_factories.factory_state(host_alloc);

    ice::Result result = ice_setup(host_alloc, *config, *state);
    while(result == ice::app::S_ApplicationSetupPending)
    {
        result = ice_setup(host_alloc, *config, *state);
    }

    ICE_LOG_IF(result == false, ice::LogSeverity::Critical, ice::LogTag::Core, "{}\n", result.error().description());
    ICE_ASSERT_CORE(result == true);

    // Before updating we need to resume first.
    if (result == ice::app::S_ApplicationUpdate)
    {
        ICE_LOG(ice::LogSeverity::Warning, ice::LogTag::Core, "An application should always move to 'Resume' after finishing the 'Setup' stage!");
        result = ice::app::S_ApplicationResume;
    }

    // We can only call exit if we are in 'suspended' state.
    // Since initially we never resumed we start with 'true'
    bool suspended_before_exit = true;

    ice::UniquePtr<ice::app::Runtime> runtime = app_factories.factory_runtime(host_alloc);
    while (result && result != ice::app::S_ApplicationExit)
    {
        // Resume the apllication (always called at least once)
        // Allows to update the application state
        if (result == ice::app::S_ApplicationResume)
        {
            suspended_before_exit = false;
            result = ice_resume(*config, *state, *runtime);
        }

        // Only enter update if resume was successful
        while(result == ice::app::S_ApplicationUpdate)
        {
            result = ice_update(*config, *state, *runtime);
        }

        // Suspend the apllication (always called at least once)
        // Allows to cleanup the application state
        if (result == ice::app::S_ApplicationSuspend)
        {
            result = ice_suspend(*config, *state, *runtime);
            suspended_before_exit = true;
        }
    }

    // We only suspend if no error was triggered.
    if (result && suspended_before_exit == false)
    {
        result = ice_suspend(*config, *state, *runtime);
    }

    runtime.reset();

    ICE_LOG_IF(result == false, ice::LogSeverity::Error, ice::LogTag::Core, "{}\n", result.error().description());
    ICE_ASSERT_CORE(result == true);

    result = ice_shutdown(host_alloc, *config, *state);
    ICE_LOG_IF(result == false, ice::LogSeverity::Error, ice::LogTag::Core, "{}\n", result.error().description());
    ICE_ASSERT_CORE(result == true);

    ice::platform::linux::sdl2::UnixWindow_X11WaylandSDL2::shutdown_subsystem();
    return 0;
}
