/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os/unix.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/container/linked_queue.hxx>
#include "native_fileio.hxx"
#include "native_aio.hxx"

namespace ice
{

#if ISP_WINDOWS
#elif ISP_UNIX

    struct AsyncIOData : public ice::TaskAwaitableBase
    {
        ice::NativeAIO* nativeio;
        ice::native_fileio::File file;
        ice::Memory destination;
        ice::ucount read_size;

        struct Result
        {
            bool success;
            ice::ucount bytes_read;
        };

        AsyncIOData(
            ice::NativeAIO* nativeio,
            ice::native_fileio::File file_handle,
            ice::Memory memory,
            ice::ucount size
        ) noexcept
            : TaskAwaitableBase{ ._params{ TaskAwaitableModifier_v3::Unused } }
            , nativeio{ nativeio }
            , file{ ice::move(file_handle) }
            , destination{ memory }
            , read_size{ size }
        {
            ICE_ASSERT(
                read_size <= memory.size.value,
                "Trying to read more than the buffer can store!"
            );
        }

        bool await_ready() const noexcept
        {
            // Don't schedule when eof reached.
            // return check_file(file, FileState::EndOfFile) == false;
            // TODO: Eearly out if EOF reached.

            // For small files it's better to not schedule the read on a separate thread.
            return ice::usize{ read_size } <= 4_KiB;
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
            Result result{ .success = false, .bytes_read = 0 };

            // We are now resumed on the new thread, so here we can read the file blocking.
            ice::usize const read_result = native_fileio::read_file(file, { read_size }, destination);

            ICE_ASSERT(read_result <= destination.size, "Read more bytes that requested!");
            result.bytes_read = static_cast<ice::ucount>(read_result.value);
            // TODO: Improve success check.
            result.success = result.bytes_read > 0;// check_file(file, FileState::ReadError) == false;
            return result;
        }
    };

#endif

} // namespace ice
