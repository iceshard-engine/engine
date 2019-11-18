#include <application/application.hxx>
#include <application/utility.hxx>

#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/pod/array.hxx>
#include <core/string.hxx>
#include <core/string_view.hxx>

#include <resource/system.hxx>
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
            auto working_dir = app::working_directory(main_allocator);
            fmt::print("Initializing filesystem module at: {}\n", working_dir);

            core::pod::Array<core::cexpr::stringid_type> schemes{ core::memory::globals::default_scratch_allocator() };
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
            using resource::URN;
            using resource::URI;

            // Dynamic Library Resources
            resource_system.mount(URI{ resource::scheme_dynlib, "bin" });

            // Default file system mount points
            resource_system.mount(URI{ resource::scheme_file, "mount.isr" });
            resource_system.mount(URI{ resource::scheme_directory, "data" });

            // Check for an user defined mounting file
            if (auto* mount_resource = resource_system.find(URI{ resource::scheme_file, "mount.isr" }))
            {
                fmt::print("Custom mount resource found: {}\n", mount_resource->location());

                // #todo Read the mount file and mount the directories there.
            }
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
    //mem::globals::init();
    //fs::filesystem::initialize(mem::globals::default_allocator());

    //MLogInfo("cwd: {}", fs::current_working_directory().c_str());

    //fs::filesystem::instance().mount("../../data"); // Static data
    //fs::filesystem::instance().mount("data_debug"); // Debug data

    //auto game = CreateGame(mem::globals::default_allocator());
    //game->OnCreate();

    //// Moving the Engine object from the static function to the stack-value
    //// Provides an automatic life cycle for the engine
    //auto* engine = EngineCore::create(mem::globals::default_allocator());

    //game->set_engine(engine);
    //game->OnInitialized();

    //// Handle events
    //while (game->HandleEvents())
    //{
    //    // Swap the 'frame' of the game (and synchronize)
    //    game->Prepare();

    //    // Start the next game update before rendering starts
    //    game->Update();

    //    // Render the game
    //    game->Render();
    //}

    //game->destroy();

    //EngineCore::destroy(engine);

    //fs::filesystem::shutdown();
    //mem::globals::shutdown();
    return result;
}
