/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_traverser.hxx"

#include <ice/task_scheduler.hxx>
#include <ice/task_utils.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/string/static_string.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>

namespace ice
{

    struct FileSystemTraverseRequest
    {
        ice::FileSystemTraverser& self;
        ice::native_file::FilePath base_path;
        std::atomic_uint32_t& remaining;
        ice::TaskScheduler* worker_thread;
        ice::TaskScheduler* final_thread;
    };

    FileSystemTraverser::FileSystemTraverser(ice::FileSystemTraverserCallbacks& callbacks) noexcept
        : _callbacks{ callbacks }
    {
    }

    void FileSystemTraverser::create_resource_from_file(
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath file_path
    ) noexcept
    {
        // Early out for metadata files.
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
        {
            return;
        }

        // Handle full .isr files
        ice::FileSystemResource* resource = nullptr;
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isr"))
        {
            resource = _callbacks.create_baked_resource(file_path);
        }
        else
        {
            ice::StackAllocator_1024 temp_alloc;
            ice::native_file::FilePath const uribase = ice::path::directory(base_path);
            ice::native_file::FilePath const datafile = file_path;
            ice::native_file::HeapFilePath metafile{ temp_alloc };
            ice::string::reserve(metafile, 512);
            ice::string::push_back(metafile, file_path);
            ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

            resource = _callbacks.create_loose_resource(
                base_path,
                uribase,
                metafile,
                datafile
            );
        }

        if (resource != nullptr)
        {
            ice::Result result = _callbacks.register_resource(resource);
            if (result == false)
            {
                ICE_LOG(
                    LogSeverity::Error, LogTag::Engine,
                    "Traverser failed to register resource in provider! {}",
                    result.error()
                );

                _callbacks.destroy_resource(resource);
            }
        }
    }

    auto FileSystemTraverser::create_resource_from_file_async(
        ice::native_file::FilePath base_path,
        ice::native_file::HeapFilePath file_path,
        ice::FileSystemTraverseRequest& request
    ) noexcept -> ice::Task<>
    {
        // Early out for metadata files.
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
        {
            request.remaining -= 1;
            co_return;
        }

        // Handle full .isr files
        ice::FileSystemResource* resource = nullptr;
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isr"))
        {
            resource = _callbacks.create_baked_resource(file_path);
        }
        else
        {
            ice::StackAllocator_1024 temp_alloc;
            ice::native_file::FilePath const uribase = ice::path::directory(base_path);
            ice::native_file::FilePath const datafile = file_path;
            ice::native_file::HeapFilePath metafile{ temp_alloc };
            ice::string::reserve(metafile, 512);
            ice::string::push_back(metafile, file_path);
            ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

            resource = _callbacks.create_loose_resource(
                base_path,
                uribase,
                metafile,
                datafile
            );
        }

        co_await *request.final_thread; // Await on a single final thread

        if (resource != nullptr)
        {
            ice::Result result = _callbacks.register_resource(resource);
            if (result == false)
            {
                ICE_LOG(
                    LogSeverity::Error, LogTag::Engine,
                    "Traverser failed to register resource in provider! {}",
                    result.error()
                );

                _callbacks.destroy_resource(resource);
            }
        }

        request.remaining -= 1;
        co_return;
    }

    auto FileSystemTraverser::traverse_async(
        ice::native_file::HeapFilePath dir_path,
        ice::FileSystemTraverseRequest& request
    ) noexcept -> ice::Task<>
    {
        IPT_ZONE_SCOPED;
        ice::native_file::traverse_directories(dir_path, traverse_callback, &request);
        request.remaining -= 1;
        co_return;
    }

    /*static*/
    auto FileSystemTraverser::traverse_callback(
        ice::native_file::FilePath,
        ice::native_file::FilePath path,
        ice::native_file::EntityType type,
        void* userdata
    ) noexcept -> ice::native_file::TraverseAction
    {
        FileSystemTraverseRequest* request = reinterpret_cast<FileSystemTraverseRequest*>(userdata);
        if (type == ice::native_file::EntityType::File)
        {
            if (request->worker_thread != nullptr)
            {
                request->remaining += 1;
                ice::Allocator& alloc = request->self._callbacks.allocator();
                ice::schedule_task(
                    request->self.create_resource_from_file_async(request->base_path, { alloc, path }, *request),
                    *request->worker_thread
                );
            }
            else
            {
                request->self.create_resource_from_file(request->base_path, path);
            }
        }
        else if (type == ice::native_file::EntityType::Directory && request->worker_thread != nullptr)
        {
            request->remaining += 1;

            ice::Allocator& alloc = request->self._callbacks.allocator();
            ice::schedule_task(
                request->self.traverse_async({ alloc, path }, *request),
                *request->worker_thread
            );

            // Since we now run sub-directories as separate tasks, we skip them here
            return ice::native_file::TraverseAction::SkipSubDir;
        }

        // Continue in synchronous scenario
        return ice::native_file::TraverseAction::Continue;
    }

    void FileSystemTraverser::initial_traverse(
        ice::Span<ice::native_file::HeapFilePath> base_paths
    ) noexcept
    {
        ice::Array<FileSystemTraverseRequest, ContainerLogic::Complex> requests{ _callbacks.allocator() };
        ice::array::reserve(requests, ice::span::count(base_paths));

        [[maybe_unused]]
        std::atomic_uint32_t remaining = 0;
        for (ice::native_file::FilePath base_path : base_paths)
        {
            ice::array::push_back(requests, { *this, base_path, remaining, nullptr, nullptr });
            ice::native_file::traverse_directories(
                base_path, traverse_callback, &ice::array::back(requests)
            );
        }
    }

    void FileSystemTraverser::initial_traverse_mt(
        ice::Span<ice::native_file::HeapFilePath> base_paths,
        ice::TaskScheduler& scheduler
    ) noexcept
    {
        ice::TaskQueue local_queue{ };
        ice::TaskScheduler local_sched{ local_queue };

        ice::Array<FileSystemTraverseRequest, ContainerLogic::Complex> requests{ _callbacks.allocator() };
        ice::array::reserve(requests, ice::span::count(base_paths));

        std::atomic_uint32_t remaining = 0;

        // Traverse directories synchronously but create resources asynchronously.
        for (ice::native_file::FilePath base_path : base_paths)
        {
            ice::array::push_back(requests, { *this, base_path, remaining, ice::addressof(scheduler), &local_sched });
            ice::native_file::traverse_directories(
                base_path, traverse_callback, &ice::array::back(requests)
            );
        }

        // Process all awaiting tasks
        while (remaining > 0)
        {
            local_queue.process_all();
        }
    }

} // namespace ice
