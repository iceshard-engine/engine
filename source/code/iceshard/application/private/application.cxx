#include <ice/application.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>

#include <ice/memory.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/string.hxx>
#include <ice/heap_string.hxx>

#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>

#include <ice/app_info.hxx>
#include <ice/log.hxx>

int main(int, char**)
{
    ice::memory::init();

    ice::i32 app_result = -1;

    // The application lifetime scope
    {
        ice::Allocator& main_allocator = ice::memory::default_allocator();

        ice::memory::ProxyAllocator filesystem_allocator{ main_allocator, "resource-filesystem" };
        ice::memory::ProxyAllocator dynlib_allocator{ main_allocator, "resource-dynlib" };

        ice::HeapString<char8_t> working_dir{ main_allocator };
        ice::working_directory(working_dir);

        ice::HeapString<char8_t> app_location{ main_allocator };
        ice::app_location(app_location);

        // TODO: allow to set working dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(working_dir, ice::string::size(working_dir) - (ice::size("build") - 1));
        ice::string::push_back(working_dir, u8"source\\data\\");
        ice::UniquePtr<ice::ResourceProvider_v2> filesys_provider = ice::create_resource_provider(filesystem_allocator, working_dir);

        // TODO: allow to set dynlib dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(app_location, ice::string::size(app_location) - (ice::size("test\\test.exe") - 1));
        ice::UniquePtr<ice::ResourceProvider_v2> dynlib_provider = ice::create_resource_provider_dlls(dynlib_allocator, app_location);

        ice::UniquePtr<ice::ResourceTracker_v2> resource_tracker = ice::create_resource_tracker(main_allocator, true);

        resource_tracker->attach_provider(filesys_provider.get());
        resource_tracker->attach_provider(dynlib_provider.get());

        // Refresh all providers
        resource_tracker->refresh_providers();

        ice::memory::ProxyAllocator game_alloc{ main_allocator, "game" };
        app_result = game_main(game_alloc, *resource_tracker);
    }

    ice::memory::shutdown();
    return app_result;
}
