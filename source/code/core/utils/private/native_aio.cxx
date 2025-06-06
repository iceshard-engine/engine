/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "native_aio.hxx"
#include <ice/assert.hxx>

namespace ice::native_aio
{

#if ISP_WINDOWS
    auto aio_open(
        ice::Allocator& alloc,
        ice::native_aio::AIOPortInfo const& info
    ) noexcept -> ice::native_aio::AIOPort
    {
        ice::u32 const worker_limit = std::max(info.worker_limit, 1u);

        AIOPortInternal* result = nullptr;
        HANDLE const completion_port = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE, NULL, NULL, worker_limit
        );
        if (completion_port != NULL)
        {
            result = alloc.create<AIOPortInternal>(alloc, completion_port, worker_limit);
        }
        return result;
    }

    auto aio_native_handle(ice::native_aio::AIOPort port) noexcept -> void*
    {
        return port ? port->_completion_port : nullptr;
    }

    auto aio_worker_limit(ice::native_aio::AIOPort port) noexcept -> ice::u32
    {
        return port ? port->_worker_limit : 0;
    }

    void aio_close(ice::native_aio::AIOPort port) noexcept
    {
        if (port != nullptr)
        {
            ice::Allocator& alloc = port->_allocator;
            CloseHandle(port->_completion_port);
            alloc.destroy(port);
        }
    }

    void aio_file_flags(
        ice::native_aio::AIOPort port,
        ice::native_file::FileOpenFlags& flags
    ) noexcept
    {
        flags |= ice::native_file::FileOpenFlags::Asynchronous;
    }

    bool aio_file_bind(
        ice::native_aio::AIOPort port,
        ice::native_file::File const& file
    ) noexcept
    {
        return CreateIoCompletionPort(
            file.native(),
            ice::native_aio::aio_native_handle(port),
            NULL, /* Don't make use of this key yet */
            NULL /* Ignored in this call, because an existing port was used */
        ) != NULL;
    }

    auto aio_file_read_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::native_file::FileRequestStatus
    {
        using ice::native_file::FileRequestStatus;

        DWORD result = FALSE;
        DWORD last_error = S_OK;
        DWORD characters_read = 0;

        // TODO: Request as many reads as necessary to read the whole requested length?
        DWORD const characters_to_read = static_cast<DWORD>(requested_read_size.value);
        ICE_ASSERT(
            characters_to_read == requested_read_size.value,
            "File is larger than this function can handle! For now... [native_file size: {}]",
            requested_read_size
        );

        // Reset the request object internal buffer and get an overlapped pointer.
        ice::memset(request._internal, 0, sizeof(request._internal));
        OVERLAPPED* overlapped = reinterpret_cast<OVERLAPPED*>(request._internal + 0);

        LARGE_INTEGER const offset{ .QuadPart = static_cast<ice::isize::base_type>(requested_read_offset.value) };
        overlapped->Offset = offset.LowPart;
        overlapped->OffsetHigh = offset.HighPart;

        result = ReadFile(
            file.native(),
            memory.location,
            characters_to_read,
            &characters_read,
            overlapped
        );
        last_error = GetLastError();

        if (result == TRUE && last_error == ERROR_SUCCESS)
        {
            return FileRequestStatus::Completed;
        }
        else if (last_error == ERROR_IO_PENDING)
        {
            return FileRequestStatus::Pending;
        }
        else
        {
            return FileRequestStatus::Error;
        }
    }

    auto aio_file_write_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_write_offset,
        ice::Data data
    ) noexcept -> ice::native_file::FileRequestStatus
    {
        using ice::native_file::FileRequestStatus;

        ice::usize total_writes_requested = 0_B;

        // Reset the request object internal buffer and get an overlapped pointer.
        ice::memset(request._internal, 0, sizeof(request._internal));
        OVERLAPPED* overlapped = reinterpret_cast<OVERLAPPED*>(request._internal + 0);

        // Set the initial offset at which we want to write.
        // NOTE: If offset is set to 0xffff'ffff'ffff'ffff == Append to end of file. From MSDN docs.
        LARGE_INTEGER const offset{ .QuadPart = (LONGLONG) requested_write_offset.value };
        overlapped->Offset = offset.LowPart;
        overlapped->OffsetHigh = offset.HighPart;

        FileRequestStatus current_status = FileRequestStatus::Pending;
        while(data.size > 0_B && current_status != FileRequestStatus::Error)
        {
            // Calculate how much data we should write in a single call (up-to DWORD max value)
            ice::usize::base_type const data_to_write = ice::min<ice::usize::base_type>(
                data.size.value, std::numeric_limits<DWORD>::max()
            );

            BOOL const result = WriteFile(file.native(), data.location, (DWORD) data_to_write, nullptr, overlapped);
            DWORD last_error = GetLastError();

            if (result == TRUE && last_error == ERROR_SUCCESS)
            {
                current_status = FileRequestStatus::Completed;
            }
            else if (last_error == ERROR_IO_PENDING)
            {
                current_status = FileRequestStatus::Pending;
            }
            else
            {
                current_status = FileRequestStatus::Error;
            }

            total_writes_requested += { data_to_write };

            // Update the offset where we should write. Unless we are always appending
            if (requested_write_offset != ice::usize{ std::numeric_limits<ice::usize::base_type>::max() })
            {
                requested_write_offset += { data_to_write };

                LARGE_INTEGER const new_offset{ .QuadPart = (LONGLONG) requested_write_offset.value };
                overlapped->Offset = new_offset.LowPart;
                overlapped->OffsetHigh = new_offset.HighPart;
            }

            // Move the memory block to the next location to be written
            data = {
                .location = ice::ptr_add(data.location, { data_to_write }),
                .size = ice::usize::subtract(data.size, { data_to_write }),
                .alignment = data.alignment
            };
        }

        // TODO: tract the number of completed requests?
        return current_status;
    }

    bool aio_file_await_request(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits,
        ice::native_aio::AIORequest const*& out_request,
        ice::usize& out_size
    ) noexcept
    {
        DWORD bytes;
        ULONG_PTR key;
        OVERLAPPED* overlapped;

        bool const iocompleted = GetQueuedCompletionStatus(
            port->_completion_port,
            &bytes,
            &key,
            &overlapped,
            limits.timeout_ms
        ) != FALSE;

        DWORD const last_error = GetLastError();

        // The port was closed, just return false == timeout.
        if (last_error == ERROR_ABANDONED_WAIT_0)
        {
            return false;
        }

        // Check the port wasn't closed! (outdated?)
        ICE_ASSERT_CORE(last_error != ERROR_ABANDONED_WAIT_0 || overlapped != nullptr || iocompleted);

        out_size = { bytes };
        out_request = reinterpret_cast<ice::native_aio::AIORequest const*>(overlapped);
        return iocompleted;
    }

    void aio_complete_request(
        ice::native_aio::AIORequest const* request,
        ice::native_aio::AIORequestResult result,
        ice::usize read_size
    ) noexcept
    {
        if (request != nullptr && request->_callback != nullptr)
        {
            request->_callback(result, read_size, request->_userdata);
        }
    }
#elif ISP_WEBAPP || ISP_ANDROID || ISP_LINUX
    auto aio_open(
        ice::Allocator& alloc,
        ice::native_aio::AIOPortInfo const& info
    ) noexcept -> ice::native_aio::AIOPort
    {
        AIOPortInternal* result = alloc.create<AIOPortInternal>(alloc);
        result->_worker_limit = std::max(info.worker_limit, 1u);

        if (sem_init(&result->_semaphore, 0 /*thread shared unnamed-semaphore*/, 0 /*start at 0*/) != 0)
        {
            alloc.destroy(result);
            return nullptr;
        }
        return result;
    }

    auto aio_native_handle(ice::native_aio::AIOPort port) noexcept -> void*
    {
        return port;
    }

    auto aio_worker_limit(ice::native_aio::AIOPort port) noexcept -> ice::u32
    {
        return port ? port->_worker_limit : 0;
    }

    void aio_close(ice::native_aio::AIOPort port) noexcept
    {
        if (port != nullptr)
        {
            sem_destroy(&port->_semaphore);
            port->_allocator.destroy(port);
        }
    }

    void aio_file_flags(
        ice::native_aio::AIOPort port,
        ice::native_file::FileOpenFlags& flags
    ) noexcept
    {
        flags &= ~ice::native_file::FileOpenFlags::Asynchronous;
    }

    bool aio_file_bind(
        ice::native_aio::AIOPort port,
        ice::native_file::File const& file
    ) noexcept
    {
        return true; // We don't bind anything to this file
    }

    auto aio_file_read_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::native_file::FileRequestStatus
    {
        ICE_ASSERT_CORE(memory.location != nullptr);
        AIORequestInternal& internal = reinterpret_cast<AIORequestInternal&>(request);
        internal.next = nullptr;
        internal.native_file_handle = file.native();
        internal.request_type = 1; // 1 = read, 2 = write
        internal.data_location = memory.location;
        internal.data_offset = static_cast<ice::u32>(requested_read_offset.value);
        internal.data_size = static_cast<ice::u32>(requested_read_size.value);
        ice::linked_queue::push(request._port->_requests, ice::addressof(internal));

        // Increment the semaphore
        sem_post(&request._port->_semaphore);
        return ice::native_file::FileRequestStatus::Pending;
    }

    bool aio_file_await_request(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits,
        ice::native_aio::AIORequest const*& out_request,
        ice::usize& out_size
    ) noexcept
    {
        ICE_ASSERT_CORE(port);

        timespec timeval;
        if (clock_gettime(CLOCK_REALTIME, &timeval) != 0)
        {
            return false;
        }

        // Set seconds to wait
        timeval.tv_sec += limits.timeout_ms / 1000;
        limits.timeout_ms -= timeval.tv_sec * 1000;
        // Set remaining to wait
        timeval.tv_nsec += limits.timeout_ms * 1000 * 1000;

        if (sem_timedwait(&port->_semaphore, &timeval))
        {
            return false;
        }

        AIORequestInternal* internal = ice::linked_queue::pop(port->_requests);
        if (internal)
        {
            out_request = reinterpret_cast<AIORequest const*>(internal);

            if (internal->request_type == 1)
            {
                // TODO: Handle sizes / offsets above 4gb
                ice::i32 const bytes_read = pread(
                    internal->native_file_handle,
                    internal->data_location,
                    internal->data_size,
                    internal->data_offset
                );

                ICE_ASSERT_CORE(bytes_read > 0); // 0 == eof, -1 == error
                ICE_ASSERT_CORE(bytes_read == internal->data_size);

                out_size = { (ice::usize::base_type) bytes_read };
            }
            else
            {
                // TODO: Write requests
                ICE_ASSERT_CORE(false);
            }
        }
        return out_size > 0_B;
    }

    void aio_complete_request(
        ice::native_aio::AIORequest const* request,
        ice::native_aio::AIORequestResult result,
        ice::usize read_size
    ) noexcept
    {
        if (request != nullptr && request->_callback != nullptr)
        {
            request->_callback(result, read_size, request->_userdata);
        }
    }
#else
    auto aio_open(
        ice::Allocator& alloc,
        ice::native_aio::AIOPortInfo const& info
    ) noexcept -> ice::native_aio::AIOPort
    {
        ice::u32 const worker_limit = std::max(info.worker_limit, 1u);
        AIOPortInternal* internal = alloc.create<AIOPortInternal>(alloc);
        internal->_semaphore = CreateSemaphore(NULL, 0, worker_limit, L"IS-AIO-Semaphore");
        internal->_worker_limit = worker_limit;
        return internal;
    }

    auto aio_native_handle(ice::native_aio::AIOPort port) noexcept -> void*
    {
        return port;
    }

    auto aio_worker_limit(ice::native_aio::AIOPort port) noexcept -> ice::u32
    {
        return port ? port->_worker_limit : 0;
    }

    void aio_close(ice::native_aio::AIOPort port) noexcept
    {
        if (port != nullptr)
        {
            ice::Allocator& alloc = port->_allocator;
            CloseHandle(port->_semaphore);
            alloc.destroy(port);
        }
    }

    void aio_file_flags(
        ice::native_aio::AIOPort port,
        ice::native_file::FileOpenFlags& flags
    ) noexcept
    {
        flags &= ~ice::native_file::FileOpenFlags::Asynchronous;
    }

    bool aio_file_bind(
        ice::native_aio::AIOPort port,
        ice::native_file::File const& file
    ) noexcept
    {
        return true; // We don't bind anything to this file
    }

    auto aio_file_read_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::native_file::FileRequestStatus
    {
        AIORequestInternal& internal = reinterpret_cast<AIORequestInternal&>(request);
        internal.native_file_handle = file.native();
        internal.data_location = memory.location;
        internal.data_offset = static_cast<ice::u32>(requested_read_offset.value);
        internal.data_size = static_cast<ice::u32>(requested_read_size.value);
        ice::linked_queue::push(request._port->_requests, ice::addressof(internal));
        ReleaseSemaphore(request._port->_semaphore, 1, NULL);
        return ice::native_file::FileRequestStatus::Pending;
    }

    bool aio_file_await_request(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits,
        ice::native_aio::AIORequest const*& out_request,
        ice::usize& out_size
    ) noexcept
    {
        DWORD result = WaitForSingleObject(port->_semaphore, limits.timeout_ms);
        if (result == ERROR_TIMEOUT)
        {
            return false;
        }

        AIORequestInternal* internal = ice::linked_queue::pop(port->_requests);
        if (internal)
        {
            out_request = reinterpret_cast<AIORequest const*>(internal);

            OVERLAPPED overlapped{};
            LARGE_INTEGER const offset{ .QuadPart = internal->data_offset };
            overlapped.Offset = offset.LowPart;
            overlapped.OffsetHigh = offset.HighPart;

            DWORD data_read = 0;
            result = ReadFile(
                internal->native_file_handle,
                internal->data_location,
                internal->data_size,
                &data_read,
                &overlapped
            );
            ICE_ASSERT_CORE(result > 0);
            ICE_ASSERT_CORE(data_read == internal->data_size);
            out_size = { data_read };
        }
        return out_size > 0_B;
    }

    void aio_complete_request(
        ice::native_aio::AIORequest const* request,
        ice::native_aio::AIORequestResult result,
        ice::usize read_size
    ) noexcept
    {
        if (request != nullptr && request->_callback != nullptr)
        {
            request->_callback(result, read_size, request->_userdata);
        }
    }
#endif

    auto aio_process_events(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits
    ) noexcept -> ice::ucount
    {
        ice::usize bytes;

        ice::ucount num_completed = 0;
        while(limits.events_max > num_completed)
        {
            ice::native_aio::AIORequest const* request = nullptr;

            // TODO: Allow different requests
            bool const iocompleted = aio_file_await_request(port, limits, request, bytes);

            // After the first successful request we will timeout on the next one immediatly if nothing else remains to be done.
            limits.timeout_ms = 0;

            if (iocompleted)
            {
                aio_complete_request(request, AIORequestResult::Success, bytes);
                num_completed += 1;
            }
            else if (request != nullptr)
            {
                aio_complete_request(request, AIORequestResult::Error, 0_B);
                num_completed += 1;
            }
            // Else it's a timeout and we break
            else
            {
                break;
            }
        }
        return num_completed;
    }

} // namespace ice::native_aio
