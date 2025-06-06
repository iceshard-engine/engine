/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/app.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/params.hxx>
#include <ice/log.hxx>

// Undef definition from SDL
#undef main

int main(int argc, char const** argv)
{
    ice::i32 app_result = 0;

    // The application lifetime scope
    {
        ice::HostAllocator host_alloc{ };

        ice::app::Factories app_factories{ };
        ice_init(host_alloc, app_factories);

        ice::Params params = ice::create_params(host_alloc, "iceshard", "0.0.1", "IceShard engine parameters");
        ice::UniquePtr<ice::app::Config> config = app_factories.factory_config(host_alloc);

        ice_args(host_alloc, params, *config);

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
    }

    return app_result;
}
