/// Copyright 2021 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/application.hxx>
#include <ice/os/windows.hxx>
#include <ice/app_info.hxx>

#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/array.hxx>
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>

#include <ice/task_thread_pool.hxx>

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
        ice::HeapString<> shader_dir{ host_alloc };
        ice::working_directory(working_dir);

        ice::HeapString<> app_location{ host_alloc };
        ice::app_location(app_location);

        // TODO: allow to set working dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(working_dir, ice::string::size(working_dir) - (ice::count("build") - 1));
        shader_dir = working_dir;

        // TODO: We need to finally allow changing or adding resource paths before we start loading the game.
        ice::string::push_back(working_dir, "source\\data\\");
        ice::string::push_back(shader_dir, "build\\obj\\VkShaders\\GFX-Vulkan-Unoptimized-vk-glslc-1-3\\data\\");

        ice::String resource_paths[]{ working_dir, shader_dir };
        ice::UniquePtr<ice::ResourceProvider> filesys_provider = ice::create_resource_provider(filesystem_allocator, resource_paths);

        // TODO: allow to set dynlib dir via arguments.
        // NOTE: this change is temporary so we don't change anything that might have been depending that we are in the 'build' directory.
        ice::string::resize(app_location, ice::string::size(app_location) - (ice::count("test\\test.exe") - 1));
        ice::UniquePtr<ice::ResourceProvider> dynlib_provider = ice::create_resource_provider_dlls(dynlib_allocator, app_location);

        ice::TaskQueue thread_pool_queue;
        ice::TaskScheduler thread_pool_scheduler{ thread_pool_queue };
        ice::TaskThreadPoolInfo_v3 const thread_pool_create{ .thread_count = 4 };
        ice::UniquePtr<ice::TaskThreadPool> thread_pool = ice::create_thread_pool(
            host_alloc,
            thread_pool_queue,
            thread_pool_create
        );

        ice::ResourceTrackerCreateInfo const resource_tracker_create{
            .predicted_resource_count = 10000,
            .io_dedicated_threads = 1,
            .flags_io_complete = ice::TaskFlags{ },
            .flags_io_wait = ice::TaskFlags{ },
        };
        ice::UniquePtr<ice::ResourceTracker> resource_tracker = ice::create_resource_tracker(
            resource_allocator,
            thread_pool_scheduler,
            resource_tracker_create
        );

        resource_tracker->attach_provider(filesys_provider.get());
        resource_tracker->attach_provider(dynlib_provider.get());

        // Refresh all providers
        resource_tracker->sync_resources();

        ice::ProxyAllocator game_alloc{ host_alloc, "game" };
        app_result = game_main(game_alloc, thread_pool_scheduler, *resource_tracker);
    }

    return app_result;
}
