/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_aio.hxx>
#include <ice/native_file.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/container/linked_queue.hxx>
#include <ice/os.hxx>

namespace ice::native_aio
{

#if ISP_WINDOWS
    struct AIOPortInternal
    {
        ice::Allocator& _allocator;
        HANDLE _completion_port;
        ice::u32 _worker_limit;
    };
#elif ISP_ANDROID || ISP_WEBAPP || ISP_LINUX
    struct AIORequestInternal
    {
        AIORequestInternal* next;
        ice::i32 native_file_handle;
        ice::u32 request_type; // 1 == read, 2 == write
        union
        {
            void* data_destination;
            void const* data_location;
        };
        ice::u32 data_offset;
        ice::u32 data_size;
    };

    struct AIOPortInternal
    {
        ice::Allocator& _allocator;
        ice::AtomicLinkedQueue<AIORequestInternal> _requests;
        sem_t _semaphore;
        ice::u32 _worker_limit;
    };

    static_assert(sizeof(AIORequestInternal) <= sizeof(AIORequest::_internal));
#else
    struct AIORequestInternal
    {
        AIORequestInternal* next;
        HANDLE native_file_handle;
        void* data_location;
        ice::u32 data_offset;
        ice::u32 data_size;
    };

    struct AIOPortInternal
    {
        ice::Allocator& _allocator;
        ice::AtomicLinkedQueue<AIORequestInternal> _requests;
        HANDLE _semaphore;
        ice::u32 _worker_limit;
    };

    static_assert(sizeof(AIORequestInternal) == sizeof(AIORequest::_internal));
#endif

    void aio_file_flags(
        ice::native_aio::AIOPort port,
        ice::native_file::FileOpenFlags& flags
    ) noexcept;

    bool aio_file_bind(
        ice::native_aio::AIOPort port,
        ice::native_file::File const& file
    ) noexcept;

    auto aio_file_read_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_read_offset,
        ice::usize requested_read_size,
        ice::Memory memory
    ) noexcept -> ice::native_file::FileRequestStatus;

    auto aio_file_write_request(
        ice::native_aio::AIORequest& request,
        ice::native_file::File const& file,
        ice::usize requested_write_offset,
        ice::Data data
    ) noexcept -> ice::native_file::FileRequestStatus;

    bool aio_file_await_request(
        ice::native_aio::AIOPort port,
        ice::native_aio::AIOProcessLimits limits,
        ice::native_aio::AIORequest const*& out_request,
        ice::usize& out_size
    ) noexcept;

    void aio_complete_request(
        ice::native_aio::AIORequest const* request,
        ice::native_aio::AIORequestResult result,
        ice::usize read_size
    ) noexcept;

} // namespace ice::native_aio
