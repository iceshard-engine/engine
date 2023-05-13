/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/windows.hxx>
#include <ice/task.hxx>
#include <ice/task_awaitable.hxx>
#include "../resource_native_thread_io.hxx"

namespace ice
{

#if ISP_WINDOWS

    struct AsyncIOData
    {
        OVERLAPPED overlapped;
        ice::coroutine_handle<> coroutine;

        ice::win32::FileHandle file;
        ice::Memory destination;

        struct Result
        {
            bool success;
            ice::ucount bytes_read;
        } result{ false, 0 };

        AsyncIOData(
            ice::NativeIO* nativeio,
            ice::win32::FileHandle file_handle,
            ice::Memory memory,
            ice::ucount size
        ) noexcept
            : overlapped{ }
            , coroutine{ }
            , file{ ice::move(file_handle) }
            , destination{ memory }
            , result{ .bytes_read = size }
        {
            result.success = CreateIoCompletionPort(
                file.native(),
                ice::nativeio_handle(nativeio),
                0,
                1
            ) != NULL;
        }

        bool await_ready() const noexcept
        {
            return result.success == false;
        }

        void await_suspend(ice::coroutine_handle<> coro) noexcept
        {
            // Save the coroutine handle first
            ice::ucount read_count = result.bytes_read;

            result.success = false;
            result.bytes_read = 0;
            coroutine = coro;

            BOOL const read_success = ReadFile(
                file.native(),
                ice::ptr_add(destination.location, 0_B),
                read_count,
                nullptr,
                &overlapped
            );

            DWORD last_error = GetLastError();

            ICE_ASSERT(
                read_success == FALSE && (last_error == ERROR_IO_PENDING || last_error == ERROR_SUCCESS),
                "Failed ASync read!"
            );
        }

        [[nodiscard]]
        auto await_resume() const noexcept -> Result
        {
            return result;
        }
    };

    auto asyncio_from_overlapped(
        OVERLAPPED* overlapped
    ) noexcept -> AsyncIOData*;

#endif

} // namespace ice
