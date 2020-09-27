#include <application/application.hxx>
#include <core/platform/utility.hxx>

#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/pod/array.hxx>
#include <core/string.hxx>
#include <core/string_view.hxx>

#include <resource/resource_system.hxx>
#include <resource/modules/dynlib_module.hxx>
#include <resource/modules/filesystem_module.hxx>

int main(int, char**)
{
    if constexpr (core::build::is_release == false)
    {
        core::memory::globals::init_with_stats();
    }
    else
    {
        core::memory::globals::init();
    }

    // The main allocator object
    auto& main_allocator = core::memory::globals::default_allocator();

    // Special proxy allocators for the game and internal systems.
    core::memory::proxy_allocator filesystem_allocator{ "resource-filesystem", main_allocator };
    core::memory::proxy_allocator dynlib_allocator{ "resource-dynlib", main_allocator };

    // Game result
    int result = -1;

    // The application lifetime scope
    {
        resource::ResourceSystem resource_system{ main_allocator };

        {
            auto working_dir = core::working_directory(main_allocator);
            fmt::print("Initializing filesystem module at: {}\n", working_dir);

            core::pod::Array<core::stringid_type> schemes{ core::memory::globals::default_scratch_allocator() };
            core::pod::array::push_back(schemes, resource::scheme_file);
            core::pod::array::push_back(schemes, resource::scheme_directory);
            resource_system.add_module(
                core::memory::make_unique<resource::ResourceModule, resource::FileSystem>(
                    main_allocator
                    , filesystem_allocator
                    , working_dir)
                , schemes
            );

            core::pod::array::clear(schemes);
            core::pod::array::push_back(schemes, resource::scheme_dynlib);
            resource_system.add_module(core::memory::make_unique<resource::ResourceModule, resource::DynLibSystem>(main_allocator, dynlib_allocator), schemes);
        }

        // Initial mount points
        {
            using resource::URI;
            using resource::URN;

            // Dynamic Library Resources
            resource_system.mount(URI{ resource::scheme_dynlib, "bin" });
        }

        // The game entry point
        {
            core::memory::proxy_allocator game_allocator{ "game", main_allocator };
            result = game_main(game_allocator, resource_system);
        }
    }

    if constexpr (core::build::is_release == false)
    {
        auto& stats_allocator = static_cast<core::memory::proxy_allocator&>(main_allocator);
        fmt::print("Memory stats:\n");
        fmt::print("- total allocation count: {}\n", stats_allocator.allocation_count());
    }

    core::memory::globals::shutdown();
    return result;
}
