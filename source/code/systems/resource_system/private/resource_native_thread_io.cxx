#include "resource_native_thread_io.hxx"
#include "windows/resource_asyncio_win32.hxx"
#include <ice/task_queue.hxx>
#include <ice/container/linked_queue.hxx>

namespace ice
{

#if ISP_WINDOWS

    struct NativeIO
    {
        ice::Allocator* nativeio_allocator;
        HANDLE completion_port;

        ~NativeIO() noexcept
        {
            CloseHandle(completion_port);
        }
    };

    void destroy_nativeio(NativeIO* nativeio) noexcept
    {
        nativeio->nativeio_allocator->destroy(nativeio);
    }

    auto create_nativeio_thread_data(
        ice::Allocator& alloc,
        ice::ucount hint_thread_count
    ) noexcept -> ice::UniquePtr<ice::NativeIO>
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

        NativeIO* const native_io = alloc.create<NativeIO>(ice::addressof(alloc), completion_port);
        return ice::make_unique<ice::NativeIO>(destroy_nativeio, native_io);
    }

    auto nativeio_handle(ice::NativeIO* nativeio) noexcept -> void*
    {
        return nativeio ? nativeio->completion_port : nullptr;
    }

    auto nativeio_thread_procedure(NativeIO* nativeio, ice::TaskQueue& queue) noexcept -> ice::u32
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
            AsyncIOData* asyncio = asyncio_from_overlapped(overlapped);
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
            ice::TaskAwaitableBase* const awaitable = ice::linked_queue::pop(queue._awaitables);
            if (awaitable != nullptr)
            {
                awaitable->_coro.resume();
            }
        }

        return 0;
    }

#else
#error "Not Implemented!"
#endif

} // namespace ice
