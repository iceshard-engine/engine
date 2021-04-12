#include <ice/application.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>

#include <ice/memory.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/string.hxx>
#include <ice/heap_string.hxx>

#include <ice/resource_system.hxx>
#include <ice/resource_index.hxx>

#include <ice/app_info.hxx>

int main(int, char**)
{
    if constexpr (ice::build::is_release == false)
    {
        ice::memory::init_with_stats();
    }
    else
    {
        ice::memory::init();
    }

    ice::Allocator& main_allocator = ice::memory::default_allocator();

    ice::memory::ProxyAllocator filesystem_allocator{ main_allocator, "resource-filesystem" };
    ice::memory::ProxyAllocator dynlib_allocator{ main_allocator, "resource-dynlib" };

    ice::i32 app_result = -1;

    // The application lifetime scope
    {
        ice::UniquePtr<ice::ResourceSystem> resource_system = ice::create_resource_system(main_allocator);

        {
            ice::HeapString<> working_dir{ main_allocator };
            ice::working_directory(working_dir);

            ice::HeapString<> app_location{ main_allocator };
            ice::app_location(app_location);

            // #todo logger
            // fmt::print("Initializing filesystem module at: {}\n", working_dir);

            ice::pod::Array<ice::StringID> schemes{ ice::memory::default_scratch_allocator() };
            ice::pod::array::push_back(schemes, ice::scheme_file);
            ice::pod::array::push_back(schemes, ice::scheme_directory);

            resource_system->register_index(
                schemes,
                ice::create_filesystem_index(filesystem_allocator, working_dir)
            );

            ice::pod::array::clear(schemes);
            ice::pod::array::push_back(schemes, ice::scheme_dynlib);

            resource_system->register_index(
                schemes,
                ice::create_dynlib_index(dynlib_allocator, app_location)
            );
        }

        using ice::operator""_uri;

        ice::u32 const mounted_modules = resource_system->mount("dynlib://./.."_uri);

        ice::memory::ProxyAllocator game_alloc{ main_allocator, "game" };
        app_result = game_main(game_alloc, *resource_system);
    }

    ice::memory::shutdown();
    return app_result;
}
