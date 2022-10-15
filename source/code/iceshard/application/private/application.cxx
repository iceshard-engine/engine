﻿#include <ice/application.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>

#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/array.hxx>
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>

#include <ice/resource_provider.hxx>
#include <ice/resource_tracker.hxx>

#include <ice/app_info.hxx>
#include <ice/log.hxx>

int main(int, char**)
{
    ice::i32 app_result = -1;

    // The application lifetime scope
    {
        ice::HostAllocator host_alloc{ };

        ice::ProxyAllocator filesystem_allocator{ host_alloc, "resource-filesystem" };
        ice::ProxyAllocator dynlib_allocator{ host_alloc, "resource-dynlib" };
        ice::ProxyAllocator resource_allocator{ host_alloc, "resources" };

        ice::HeapString<> working_dir{ host_alloc };
        ice::working_directory(working_dir);

        ice::HeapString<> app_location{ host_alloc };
        ice::app_location(app_location);

        // TODO: allow to set working dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(working_dir, ice::string::size(working_dir) - (ice::count("build") - 1));
        ice::string::push_back(working_dir, "source\\data\\");
        ice::UniquePtr<ice::ResourceProvider> filesys_provider = ice::create_resource_provider(filesystem_allocator, working_dir);

        // TODO: allow to set dynlib dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(app_location, ice::string::size(app_location) - (ice::count("test\\test.exe") - 1));
        ice::UniquePtr<ice::ResourceProvider> dynlib_provider = ice::create_resource_provider_dlls(dynlib_allocator, app_location);

        ice::UniquePtr<ice::ResourceTracker> resource_tracker = ice::create_resource_tracker(resource_allocator, { .create_loader_thread = true });

        resource_tracker->attach_provider(filesys_provider.get());
        resource_tracker->attach_provider(dynlib_provider.get());

        // Refresh all providers
        resource_tracker->refresh_providers();

        ice::ProxyAllocator game_alloc{ host_alloc, "game" };
        app_result = game_main(game_alloc, *resource_tracker);
    }

    return app_result;
}
