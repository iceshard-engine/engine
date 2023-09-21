/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "native_aio.hxx"
#include "native_aio_tasks.hxx"
#include <ice/container/linked_queue.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/os/windows.hxx>

namespace ice
{

#if ISP_WINDOWS

    struct NativeAIO
    {
        ice::Allocator* nativeio_allocator;
        HANDLE completion_port;

        ~NativeAIO() noexcept
        {
            CloseHandle(completion_port);
        }
    };

    void destroy_nativeio(NativeAIO* nativeio) noexcept
    {
        nativeio->nativeio_allocator->destroy(nativeio);
    }

    auto create_nativeio_thread_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& /*default_scheduler*/,
        ice::ucount hint_thread_count
    ) noexcept -> ice::UniquePtr<ice::NativeAIO>
    {
        HANDLE completion_port = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            NULL,
            0,
            hint_thread_count
        );

        if (completion_port == NULL)
        {
            return {};
        }

        NativeAIO* const native_io = alloc.create<NativeAIO>(
            ice::addressof(alloc),
            completion_port
        );
        return ice::make_unique<ice::NativeAIO>(destroy_nativeio, native_io);
    }

    auto nativeio_handle(ice::NativeAIO* nativeio) noexcept -> void*
    {
        return nativeio ? nativeio->completion_port : nullptr;
    }

    auto nativeio_thread_procedure(
        ice::NativeAIO* nativeio,
        ice::TaskQueue& queue
    ) noexcept -> ice::u32
    {
        DWORD bytes;
        ULONG_PTR key = 0;
        OVERLAPPED* overlapped;

        bool const iocompleted = GetQueuedCompletionStatus(
            nativeio->completion_port,
            &bytes,
            &key,
            &overlapped,
            10
        ) != FALSE;

        DWORD last_error = GetLastError();
        last_error = 0;
        if (iocompleted)
        {
            AsyncIOData* const asyncio = asyncio_from_overlapped(overlapped);
            asyncio->result.success = true;
            asyncio->result.bytes_read = bytes;
            asyncio->coroutine.resume();
        }
        else if (overlapped != nullptr)
        {
            ICE_ASSERT(false, "Failed");
        }
        else
        {
            ICE_ASSERT(key == 0, "Failed");
            ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(
                queue._awaitables
            );
            if (awaitable != nullptr)
            {
                awaitable->_coro.resume();
            }
        }

        return 0;
    }

#elif ISP_ANDROID

    struct NativeAIO
    {
        ice::TaskQueue loader_queue;
        ice::UniquePtr<ice::TaskThreadPool> loader_threads;

        ice::Allocator* nativeio_allocator = nullptr;
        // std::atomic_uint32_t request_counter = 0;

        NativeAIO(ice::Allocator* alloc) noexcept { }
        ~NativeAIO() noexcept { }
    };

    void destroy_nativeio(NativeAIO* nativeio) noexcept
    {
        nativeio->nativeio_allocator->destroy(nativeio);
    }

    auto create_nativeio_thread_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& default_scheduler,
        ice::ucount hint_thread_count
    ) noexcept -> ice::UniquePtr<ice::NativeAIO>
    {
        NativeAIO* const native_io = alloc.create<NativeAIO>(ice::addressof(alloc));

        ice::TaskThreadPoolCreateInfo const tpci{
            .thread_count = ice::max(hint_thread_count / 2u, 1u),
            .debug_name_format = "ice.loader {}",
        };

        native_io->nativeio_allocator = &alloc;
        native_io->loader_threads = ice::create_thread_pool(
            alloc,
            native_io->loader_queue,
            tpci
        );

        return ice::make_unique<ice::NativeAIO>(destroy_nativeio, native_io);
    }

    auto nativeio_handle(ice::NativeAIO* nativeio) noexcept -> void*
    {
        return ice::addressof(nativeio->loader_queue);
    }

    auto nativeio_thread_procedure(NativeAIO* nativeio, ice::TaskQueue& queue) noexcept -> ice::u32
    {
        for (auto const& item : ice::linked_queue::consume(queue._awaitables))
        {
            item->_coro.destroy();
        }
        return 0;
    }

#else
#error "Not Implemented!"
#endif

} // namespace ice
