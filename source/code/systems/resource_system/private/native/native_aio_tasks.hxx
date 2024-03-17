/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/unix.hxx>
#include <ice/os/windows.hxx>
#include <ice/native_file.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>
#include "native_aio.hxx"

namespace ice
{

#if ISP_WINDOWS

    struct FileRequest
    {
        OVERLAPPED overlapped;
        ice::coroutine_handle<> coroutine;
        ice::Memory destination;

        struct Result
        {
            ice::usize bytes_read;
        } result;

        FileRequest(
            ice::Memory memory,
            ice::usize size,
            ice::usize read_offset
        ) noexcept
            : overlapped{ }
            , coroutine{ }
            , destination{ memory }
            , result{ .bytes_read = memory.location == nullptr ? 0_B : size }
        {
            LARGE_INTEGER const offset{ .QuadPart = static_cast<ice::isize::base_type>(read_offset.value) };
            overlapped.Offset = offset.LowPart;
            overlapped.OffsetHigh = offset.HighPart;
        }

        bool await_ready() const noexcept
        {
            // We reuse the .bytes_read multiple times
            return result.bytes_read == 0_B;
        }

        [[nodiscard]]
        auto await_resume() const noexcept -> Result
        {
            return result;
        }
    };

    template<typename FileHandleType>
    struct AsyncReadRequestBase : FileRequest
    {
        FileHandleType file;

        AsyncReadRequestBase(
            ice::NativeAIO* nativeio,
            FileHandleType file_handle,
            ice::Memory memory,
            ice::usize size,
            ice::usize read_offset = 0_B
        ) noexcept
            : FileRequest{ memory, size, read_offset }
            , file{ ice::move(file_handle) }
        {
            ICE_ASSERT_CORE(size.value <= ice::u32_max);

            // Make use of the Internal until we actually call the read operation, needs to be cleared!
            overlapped.Internal = nativeio != nullptr;
            if (nativeio != nullptr)
            {
                bool const port_created = CreateIoCompletionPort(
                    file.native(),
                    ice::nativeio_handle(nativeio),
                    0,
                    1
                ) != NULL;

                // If port creation failed, bytes `read == 0` => 'failure'
                result.bytes_read *= ice::u32(port_created);
            }
        }

        bool await_suspend(ice::coroutine_handle<> coro) noexcept
        {
            coroutine = coro;

            bool const is_synchronous = ice::exchange(overlapped.Internal, 0) == 0;
            if (is_synchronous)
            {
                overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
            }

            BOOL read_result = ReadFile(
                file.native(),
                ice::ptr_add(destination.location, 0_B),
                ice::u32(ice::exchange(result.bytes_read, 0_B).value),
                nullptr,
                &overlapped
            );

            if (is_synchronous && read_result == FALSE && GetLastError() == ERROR_IO_PENDING)
            {
                DWORD bytes_read = 0;
                read_result = GetOverlappedResult(file.native(), &overlapped, &bytes_read, TRUE);
                ICE_ASSERT_CORE(read_result != FALSE);
                //DWORD const wait_result = WaitForSingleObject(overlapped.hEvent, INFINITE);
                //ICE_ASSERT_CORE(wait_result == WAIT_OBJECT_0);

                result.bytes_read = { bytes_read };
                CloseHandle(overlapped.hEvent);
            }
            else
            {
                DWORD last_error = GetLastError();
                ICE_ASSERT(
                    read_result == FALSE && (last_error == ERROR_IO_PENDING || last_error == ERROR_SUCCESS),
                    "Failed ASync read!"
                );
            }

            // If async then the coroutine is scheduled to be resumed somewhere else
            return is_synchronous == false;
        }
    };

    using AsyncReadFile = AsyncReadRequestBase<ice::native_file::File>;
    using AsyncReadFileRef = AsyncReadRequestBase<ice::native_file::File const&>;

    auto request_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> FileRequest*;

#elif ISP_ANDROID || ISP_WEBAPP

    template<typename FileHandleType>
    struct AsyncReadRequestBase : ice::TaskAwaitableBase
    {
        ice::NativeAIO* nativeio;
        FileHandleType file;
        ice::Memory destination;
        ice::usize read_offset;

        struct Result
        {
            ice::usize bytes_read;
        };

        AsyncReadRequestBase(
            ice::NativeAIO* nativeio,
            FileHandleType file_handle,
            ice::Memory memory,
            ice::usize size,
            ice::usize offset = 0_B
        ) noexcept
            : TaskAwaitableBase{ ._params{ TaskAwaitableModifier::CustomValue, { ice::u32(size.value) } } }
            , nativeio{ nativeio }
            , file{ ice::move(file_handle) }
            , destination{ memory }
            , read_offset{ offset }
        {
            ICE_ASSERT(
                _params.u32_value <= memory.size.value,
                "Trying to read more than the buffer can store!"
            );
        }

        bool await_ready() const noexcept
        {
            // Don't schedule when eof reached.
            // return check_file(file, FileState::EndOfFile) == false;
            // TODO: Eearly out if EOF reached.

            // If we got a synchronous read request (nativeio == nullptr) or the read amount
            //   is small, it's better to not schedule the read on a separate thread.
            return nativeio == nullptr || ice::usize{ _params.u32_value } <= 4_KiB;
        }

        void await_suspend(ice::coroutine_handle<> coro) noexcept
        {
            _coro = coro;

            ice::TaskQueue* const ioqueue = reinterpret_cast<ice::TaskQueue*>(nativeio_handle(nativeio));
            ice::linked_queue::push(ioqueue->_awaitables, this);
        }

        [[nodiscard]]
        auto await_resume() const noexcept -> Result
        {
            Result result{ .bytes_read = 0_B };

            // We are now resumed on the new thread, so here we can read the file blocking.
            ice::usize const read_result = native_file::read_file(file, read_offset, { _params.u32_value }, destination);

            ICE_ASSERT(read_result <= destination.size, "Read more bytes that requested!");
            result.bytes_read = read_result;
            return result;
        }
    };

    using AsyncReadFile = AsyncReadRequestBase<ice::native_file::File>;
    using AsyncReadFileRef = AsyncReadRequestBase<ice::native_file::File const&>;

#else
#error "NOT IMPLEMENTED"
#endif

} // namespace ice
