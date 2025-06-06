/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include "hsc_packer_aiostream.hxx"

#include <ice/task.hxx>
#include <ice/task_utils.hxx>
#include <ice/task_thread_utils.hxx>
#include <ice/resource_tracker.hxx>

using ice::LogSeverity;

struct HailstormAllocator final : hailstorm::Allocator
{
    HailstormAllocator(ice::Allocator& alloc) noexcept
        : _backing{ alloc }
    { }

    auto allocate(size_t size) noexcept -> hailstorm::Memory override
    {
        ice::Memory const result = _backing.allocate(ice::usize{ size });
        return { result.location, result.size.value, (size_t)result.alignment };
    }

    void deallocate(void* ptr) noexcept override
    {
        _backing.deallocate(ptr);
    }

    ice::Allocator& _backing;
};

struct HailstormAIOWriter
{
    ice::native_file::FilePath _filepath{};
    ice::native_file::File _file{};
    ice::native_aio::AIOPort _aioport;
    ice::TaskScheduler& _scheduler;

    ice::ResourceTracker& _resource_tracker;
    ice::Span<ice::ResourceHandle> _resources;

    std::atomic_uint32_t _started_writes;
    std::atomic_uint32_t _finished_loads;
    std::atomic_uint32_t _finished_writes;

    ~HailstormAIOWriter() noexcept = default;

    inline bool open_and_resize(ice::usize total_size) noexcept;
    inline bool write_header(hailstorm::Data data, ice::usize offset) noexcept;
    inline bool write_metadata(hailstorm::Data data, ice::usize offset) noexcept;
    inline bool write_resource(ice::u32 idx, ice::usize offset) noexcept;
    inline bool close() noexcept;

private:
    inline auto async_write_header(hailstorm::Data data, ice::usize offset) noexcept -> ice::Task<>;
    inline auto async_write_metadata(hailstorm::Data data, ice::usize offset) noexcept -> ice::Task<>;
    inline auto async_write_resource(ice::u32 idx, ice::usize offset) noexcept -> ice::Task<>;

    inline auto async_write(
        ice::usize write_offset,
        hailstorm::Data data
    ) noexcept;
};

inline auto HailstormAIOWriter::async_write(ice::usize write_offset, hailstorm::Data data) noexcept
{
    struct Awaitable : ice::native_aio::AIORequest
    {
        ice::native_file::File& _file;
        ice::isize const _offset;
        hailstorm::Data const _data;
        std::coroutine_handle<> coroutine;

        static void aio_read_request_callback(
            ice::native_aio::AIORequestResult result,
            ice::usize bytes_read,
            void* userdata
        ) noexcept
        {
            Awaitable* const req = reinterpret_cast<Awaitable*>(userdata);
            req->coroutine.resume();
        }


        inline Awaitable(
            ice::native_aio::AIOPort aioport,
            ice::native_file::File& file,
            ice::isize offset,
            hailstorm::Data data
        ) noexcept
            : _file{ file }
            , _offset{ offset }
            , _data{ data }
        {
            _callback = aio_read_request_callback;
            _userdata = this;
            _port = aioport;
        }

        inline bool await_ready() const noexcept { return false; }
        inline bool await_suspend(std::coroutine_handle<> coro_handle) noexcept
        {
            ICE_ASSERT_CORE(_data.size <= std::numeric_limits<ice::u32>::max());
            IPT_ZONE_SCOPED_NAMED("AsyncStream::async_write");

            // Need to set the coroutine before calling Write, since we could already be finishing writing on a different thread
            //   before we get to set this pointer after calling WriteFile
            coroutine = coro_handle;

            ice::native_file::FileRequestStatus const status = ice::native_file::write_file_request(
                *this, _file, _offset.to_usize(), { _data.location, _data.size, (ice::ualign) _data.align }
            );
            ICE_ASSERT_CORE(status != ice::native_file::FileRequestStatus::Error);
            return status != ice::native_file::FileRequestStatus::Completed;
        }

        inline bool await_resume() const noexcept { return true; }
    };

    return Awaitable{ _aioport, _file, write_offset, data };
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
    hailstorm::Data data,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_header(data, offset);
}

auto stream_write_metadata(
    hailstorm::v1::HailstormWriteData const& write_data,
    ice::u32 idx,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_metadata(write_data.metadata[idx], offset);
}

auto stream_write_resource(
    hailstorm::v1::HailstormWriteData const& write_data,
    hailstorm::v1::HailstormWriteInfo& write_info,
    ice::usize offset,
    HailstormAIOWriter* writer
) noexcept -> bool
{
    return writer->write_resource(write_info.resource_index, offset);
}

bool hscp_write_hailstorm_file(
    ice::Allocator& alloc,
    HSCPWriteParams const& params,
    hailstorm::v1::HailstormWriteData const& data,
    ice::ResourceTracker& tracker,
    ice::Span<ice::ResourceHandle> resources
) noexcept
{
    using ice::operator""_MiB;
    using namespace hailstorm::v1;

    HailstormChunk const initial_chunks[]{
        HailstormChunk{
            .size = (16_MiB).value,
            .align = 8,
            .type = 2,
            .persistance = 3,
            .app_custom_value = 42,
        },
        HailstormChunk{
            .size = (2_MiB).value,
            .align = 8,
            .type = 1,
            .persistance = 3,
            .app_custom_value = 24,
        }
    };

    HailstormAllocator hsalloc{ alloc };
    HailstormAIOWriter writer{ params.filename, {}, params.aioport, params.task_scheduler, tracker, resources };
    HailstormAsyncWriteParams const hsparams{
        .base_params = HailstormWriteParams{
            .temp_alloc = hsalloc,
            .cluster_alloc = hsalloc,
            .initial_chunks = initial_chunks,
            .estimated_chunk_count = 5,
            .fn_select_chunk = params.fn_chunk_selector,
            .fn_create_chunk = params.fn_chunk_create,
            .userdata = params.ud_chunk_selector
        },
        .fn_async_open = (HailstormAsyncWriteParams::AsyncOpenFn*) stream_open,
        .fn_async_write_header = (HailstormAsyncWriteParams::AsyncWriteHeaderFn*) stream_write_header,
        .fn_async_write_metadata = (HailstormAsyncWriteParams::AsyncWriteMetadataFn*) stream_write_metadata,
        .fn_async_write_resource = (HailstormAsyncWriteParams::AsyncWriteDataFn*) stream_write_resource,
        .fn_async_close = (HailstormAsyncWriteParams::AsyncCloseFn*) stream_close,
        .async_userdata = &writer
    };

    return hailstorm::v1::write_cluster_async(hsparams, data);
}

inline bool HailstormAIOWriter::open_and_resize(ice::usize total_size) noexcept
{
    using ice::operator|;
    using enum ice::native_file::FileOpenFlags;
    _file = ice::native_file::open_file(_aioport, _filepath, Write | Exclusive | Asynchronous).value();

#if ISP_WINDOWS
    // Resize the file
    SetFilePointerEx(_file.native(), { .QuadPart = ice::isize(total_size).value }, NULL, FILE_BEGIN);
    SetEndOfFile(_file.native());
    SetFilePointerEx(_file.native(), { 0 }, NULL, FILE_BEGIN);
#elif ISP_LINUX
    ftruncate64(_file.native(), total_size.value);
#else
    ICE_ASSER_CORE(false);
#endif

    // Create the completion port
    return true;
}

inline bool HailstormAIOWriter::write_header(
    hailstorm::Data header_data,
    ice::usize write_offset
) noexcept
{
    ice::schedule_task(async_write_header(header_data, write_offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::write_metadata(hailstorm::Data meta, ice::usize offset) noexcept
{
    ice::schedule_task(async_write_metadata(meta, offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::write_resource(ice::u32 idx, ice::usize offset) noexcept
{
    ice::schedule_task(async_write_resource(idx, offset), _scheduler);
    return true;
}

inline bool HailstormAIOWriter::close() noexcept
{
    while (_finished_writes != _started_writes)
    {
        using ice::operator""_Tms;
        ice::current_thread::sleep(5_Tms);
    }

    return true;
}

inline auto HailstormAIOWriter::async_write_header(hailstorm::Data data, ice::usize offset) noexcept -> ice::Task<>
{
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    bool const success = co_await async_write(offset, data);
    ICE_ASSERT(success, "Failed to write header data!");
    _finished_writes.fetch_add(1, std::memory_order_relaxed);
}

inline auto HailstormAIOWriter::async_write_metadata(hailstorm::Data data, ice::usize offset) noexcept -> ice::Task<>
{
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    bool const success = co_await async_write(offset, data);
    ICE_ASSERT(success, "Failed to write header data!");
    _finished_writes.fetch_add(1, std::memory_order_relaxed);
}

inline auto data_to_hsdata(ice::Data data) noexcept -> hailstorm::Data
{
    return { data.location, data.size.value, (size_t)data.alignment };
}

inline auto HailstormAIOWriter::async_write_resource(ice::u32 idx, ice::usize offset) noexcept -> ice::Task<>
{
    _started_writes.fetch_add(1, std::memory_order_relaxed);
    ice::ResourceResult const load_result = co_await _resource_tracker.load_resource(_resources[idx]);
    _finished_loads.fetch_add(1, std::memory_order_relaxed);
    if (load_result.resource_status == ice::ResourceStatus::Loaded)
    {
        bool const success = co_await async_write(offset, data_to_hsdata(load_result.data));
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
