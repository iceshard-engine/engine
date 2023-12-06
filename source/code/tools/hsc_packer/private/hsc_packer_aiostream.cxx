/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include "hsc_packer_aiostream.hxx"

#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/resource_tracker.hxx>

using ice::LogSeverity;

#define HSCP_ERROR_IF(condition, format, ...) ICE_LOG_IF(condition, LogSeverity::Error, LogTag_Main, format, __VA_ARGS__)

struct HailstormAIOWriter
{
    ice::native_file::FilePath _filepath{};
    ice::native_file::File _file{};
    ice::TaskScheduler& _scheduler;

    ice::ResourceTracker& _resource_tracker;
    ice::Span<ice::ResourceHandle*> _resources;

    std::atomic_uint32_t _started_writes;
    std::atomic_uint32_t _finished_writes;

    HANDLE _aio_completion_port = NULL;

    struct OverlappedCoroutine
    {
        OVERLAPPED _overlapped{};
        std::coroutine_handle<> _coroutine = nullptr;
    };

    ~HailstormAIOWriter() noexcept = default;

    inline bool open_and_resize(ice::usize total_size) noexcept;
    inline bool write_header(ice::Data data, ice::usize offset) noexcept;
    inline bool write_metadata(ice::Metadata idx, ice::usize offset) noexcept;
    inline bool write_resource(ice::u32 idx, ice::usize offset) noexcept;
    inline bool close() noexcept;

private:
    inline auto async_write_header(ice::Data data, ice::usize offset) noexcept -> ice::Task<>;
    inline auto async_write_metadata(ice::Metadata idx, ice::usize offset) noexcept -> ice::Task<>;
    inline auto async_write_resource(ice::u32 idx, ice::usize offset) noexcept -> ice::Task<>;

    inline auto async_write(
        ice::usize write_offset,
        ice::Data data
    ) noexcept;
};

inline auto HailstormAIOWriter::async_write(ice::usize write_offset, ice::Data data) noexcept
{
    struct Awaitable : OverlappedCoroutine
    {
        ice::native_file::File& _file;
        ice::isize const _offset;
        ice::Data const _data;

        inline Awaitable(
            ice::native_file::File& file,
            ice::isize offset,
            ice::Data data
        ) noexcept
            : _file{ file }
            , _offset{ offset }
            , _data{ data }
        { }

        inline bool await_ready() const noexcept { return false; }
        inline bool await_suspend(std::coroutine_handle<> coro_handle) noexcept
        {
            ICE_ASSERT_CORE(_data.size.value <= MAXDWORD);
            IPT_ZONE_SCOPED_NAMED("AsyncStream::async_write");

            // Need to set the coroutine before calling Write, since we could already be finishing writing on a different thread
            //   before we get to set this pointer after calling WriteFile
            _coroutine = coro_handle;

            LARGE_INTEGER const large_int{ .QuadPart = _offset.value };
            _overlapped.Offset = large_int.LowPart;
            _overlapped.OffsetHigh = large_int.HighPart;

            BOOL const result = WriteFile(
                _file.native(),
                _data.location,
                (DWORD)_data.size.value,
                NULL,
                &_overlapped
            );

            // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
            if ((result == TRUE) || (GetLastError() != ERROR_IO_PENDING))
            {
                _coroutine = nullptr;
            }
            return _coroutine != nullptr;
        }
        inline bool await_resume() const noexcept
        {
            return _coroutine != nullptr;
        }
    };

    return Awaitable{ _file, write_offset, data };
}

auto stream_open(ice::usize total_size, HailstormAIOWriter* writer) noexcept -> bool
{
    return writer->open_and_resize(total_size);
}

auto stream_close(HailstormAIOWriter* writer) noexcept -> bool
{
    return writer->close();
}

auto stream_write_header(
    ice::Data data,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_header(data, offset);
}

auto stream_write_metadata(
    ice::hailstorm::v1::HailstormWriteData const& write_data,
    ice::u32 idx,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_metadata(write_data.metadata[idx], offset);
}

auto stream_write_resource(
    ice::hailstorm::v1::HailstormWriteData const& write_data,
    ice::u32 idx,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_resource(idx, offset);
}

inline bool hscp_write_hailstorm_file(
    ice::Allocator& alloc,
    HSCPWriteParams const& params,
    ice::hailstorm::v1::HailstormWriteData const& data,
    ice::ResourceTracker& tracker,
    ice::Span<ice::ResourceHandle*> resources
) noexcept
{
    using ice::operator""_MiB;
    using namespace ice::hailstorm::v1;

    HailstormChunk const initial_chunks[]{
        HailstormChunk{
            .size = 16_MiB,
            .align = ice::ualign::b_8,
            .type = 2,
            .persistance = 3,
            .is_encrypted = false,
            .is_compressed = false,
            .app_custom_value = 42,
        },
        HailstormChunk{
            .size = 2_MiB,
            .align = ice::ualign::b_8,
            .type = 1,
            .persistance = 3,
            .is_encrypted = false,
            .is_compressed = false,
            .app_custom_value = 24,
        }
    };

    HailstormAIOWriter writer{ params.filename, {}, params.task_scheduler, tracker, resources };
    HailstormAsyncWriteParams const hsparams{
        .base_params = HailstormWriteParams{
            .temp_alloc = alloc,
            .cluster_alloc = alloc,
            .initial_chunks = initial_chunks,
            .estimated_chunk_count = 5,
            .fn_select_chunk = params.fn_chunk_selector,
            .userdata = params.ud_chunk_selector
        },
        .fn_async_open = (HailstormAsyncWriteParams::AsyncOpenFn*) stream_open,
        .fn_async_write_header = (HailstormAsyncWriteParams::AsyncWriteHeaderFn*) stream_write_header,
        .fn_async_write_metadata = (HailstormAsyncWriteParams::AsyncWriteDataFn*) stream_write_metadata,
        .fn_async_write_resource = (HailstormAsyncWriteParams::AsyncWriteDataFn*) stream_write_resource,
        .fn_async_close = (HailstormAsyncWriteParams::AsyncCloseFn*) stream_close,
        .async_userdata = &writer
    };

    return ice::hailstorm::v1::write_cluster_async(hsparams, data);
}

inline bool HailstormAIOWriter::open_and_resize(ice::usize total_size) noexcept
{
    //ICE_ASSERT_CORE(ice::path::is_absolute(_filepath));

    using ice::operator|;
    using enum ice::native_file::FileOpenFlags;
    _file = ice::native_file::open_file(_filepath, Write | Exclusive | Asynchronous);

    // Resize the file
    SetFilePointerEx(_file.native(), { .QuadPart = ice::isize(total_size).value }, NULL, FILE_BEGIN);
    SetEndOfFile(_file.native());
    SetFilePointerEx(_file.native(), { 0 }, NULL, FILE_BEGIN);

    // Create the completion port
    _aio_completion_port = CreateIoCompletionPort(_file.native(), NULL, 0, 1);
    return _aio_completion_port != NULL;
}

inline bool HailstormAIOWriter::write_header(
    ice::Data header_data,
    ice::usize write_offset
) noexcept
{
    ice::schedule_task_on(async_write_header(header_data, write_offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::write_metadata(ice::Metadata meta, ice::usize offset) noexcept
{
    ice::schedule_task_on(async_write_metadata(meta, offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::write_resource(ice::u32 idx, ice::usize offset) noexcept
{
    ice::schedule_task_on(async_write_resource(idx, offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::close() noexcept
{
    while (_finished_writes != _started_writes)
    {
        DWORD bytes_written;
        OVERLAPPED* overlapped;
        ULONG_PTR key;
        DWORD const result = GetQueuedCompletionStatus(
            _aio_completion_port,
            &bytes_written,
            &key,
            &overlapped,
            INFINITE
        );
        ICE_ASSERT_CORE(result != FALSE);

        // Resume the coroutine
        OverlappedCoroutine* ov_coro = (OverlappedCoroutine*)overlapped;
        ov_coro->_coroutine.resume();
    }

    CloseHandle(_aio_completion_port);
    return true;
}

inline auto HailstormAIOWriter::async_write_header(ice::Data data, ice::usize offset) noexcept -> ice::Task<>
{
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    bool const success = co_await async_write(offset, data);
    ICE_ASSERT(success, "Failed to write header data!");
    _finished_writes.fetch_add(1, std::memory_order_relaxed);
}

inline auto HailstormAIOWriter::async_write_metadata(ice::Metadata meta, ice::usize offset) noexcept -> ice::Task<>
{
    //ice::Metadata meta;
    //ice::Result const load_result = co_await ice::resource_meta(_resources[idx], meta);
    //HSCP_ERROR_IF(
    //    !load_result,
    //    "Failed to load resource metadata for '{}'",
    //    ice::resource_path(_resources[idx])
    //);
    //if (load_result)
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    {
        alignas(8) char temp_buffer[1024 * 4];
        if (ice::usize stored = ice::meta_store(meta, { temp_buffer, sizeof(temp_buffer), ice::ualign::b_8 }); stored.value > 0)
        {
            bool const success = co_await async_write(offset, { temp_buffer, stored, ice::ualign::b_8 });
            ICE_ASSERT(
                success,
                "Failed to write metadata."
                //ice::resource_path(_resources[idx])
            );
        }
    }
    _finished_writes.fetch_add(1, std::memory_order_relaxed);
}

inline auto HailstormAIOWriter::async_write_resource(ice::u32 idx, ice::usize offset) noexcept -> ice::Task<>
{
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    ice::ResourceResult const load_result = co_await _resource_tracker.load_resource(_resources[idx]);
    if (load_result.resource_status == ice::ResourceStatus::Loaded)
    {
        bool const success = co_await async_write(offset, load_result.data);
        ICE_ASSERT(
            success,
            "Failed to write resource data for '{}'!",
            ice::resource_path(_resources[idx])
        );

        // Release the resource now
        co_await _resource_tracker.unload_resource(_resources[idx]);
    }
    _finished_writes.fetch_add(1, std::memory_order_relaxed);
}
