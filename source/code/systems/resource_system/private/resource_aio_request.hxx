/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_aio.hxx>
#include <ice/native_file.hxx>
#include <ice/task_types.hxx>
#include <ice/profiler.hxx>

namespace ice::detail
{

    static void aio_request_callback(
        ice::native_aio::AIORequestResult result,
        ice::usize bytes_read,
        void* userdata
    ) noexcept;

    struct AsyncReadRequest : ice::native_aio::AIORequest
    {
        ice::coroutine_handle<> coroutine;
        ice::native_file::File const& file;
        ice::usize size;
        ice::usize offset;
        ice::Memory memory;

        AsyncReadRequest(
            ice::native_aio::AIOPort port,
            ice::native_file::File const& file,
            ice::usize size,
            ice::usize offset,
            ice::Memory memory
        ) noexcept
            : AIORequest{ ._port = port, ._callback = aio_request_callback, ._userdata = this }
            , file{ file }
            , size{ size }
            , offset{ offset }
            , memory{ memory }
        {
        }

        inline bool await_ready() const noexcept
        {
            return false;
        }

        inline void await_suspend(std::coroutine_handle<> coro) noexcept
        {
            coroutine = coro;
            ice::native_file::read_file_request(*this, file, offset, size, memory);
        }

        inline auto await_resume() const noexcept -> ice::usize
        {
            return size;
        }
    };

    static void aio_request_callback(
        ice::native_aio::AIORequestResult result,
        ice::usize bytes_read,
        void* userdata
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        AsyncReadRequest* const req = reinterpret_cast<AsyncReadRequest*>(userdata);
        req->size = bytes_read;
        req->coroutine.resume();
    }

} // namespace ice
