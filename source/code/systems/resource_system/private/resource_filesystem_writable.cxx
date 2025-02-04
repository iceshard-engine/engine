/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_writable.hxx"
#include "resource_aio_request.hxx"

#include <ice/config.hxx>
#include <ice/config/config_builder.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/path_utils.hxx>
#include <ice/task_utils.hxx>
#include <ice/task.hxx>

namespace ice
{

    namespace detail
    {

        auto async_file_read(
            ice::native_file::File file,
            ice::usize filesize,
            ice::native_aio::AIOPort aioport,
            ice::Memory data
        ) noexcept -> ice::Task<bool>;

        auto async_file_load(
            ice::Allocator& alloc,
            ice::Memory target_memory,
            ice::native_aio::AIOPort aioport,
            ice::String filepath,
            bool readmeta
        ) noexcept -> ice::Task<ice::Result>;

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::Memory target_memory,
            ice::String filepath,
            bool readmeta
        ) noexcept -> ice::Result;

        bool load_metadata_file(
            ice::Allocator& alloc,
            ice::native_file::FilePath path,
            ice::Memory& out_memory,
            ice::ConfigBuilder& out_metadata
        ) noexcept;

        struct WriterAwaitable : ice::native_aio::AIORequest
        {
            ice::native_file::File& _file;
            ice::isize const _offset;
            ice::Data const _data;
            std::coroutine_handle<> coroutine;

            static void aio_read_request_callback(
                ice::native_aio::AIORequestResult result,
                ice::usize bytes_read,
                void* userdata
            ) noexcept
            {
                WriterAwaitable* const req = reinterpret_cast<WriterAwaitable*>(userdata);
                req->coroutine.resume();
            }

            inline WriterAwaitable(
                ice::native_aio::AIOPort aioport,
                ice::native_file::File& file,
                ice::isize offset,
                ice::Data data
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
                ICE_ASSERT_CORE(_data.size <= ice::usize{ice::u32_max});
                IPT_ZONE_SCOPED_NAMED("AsyncStream::async_write");

                // Need to set the coroutine before calling Write, since we could already be finishing writing on a different thread
                //   before we get to set this pointer after calling WriteFile
                coroutine = coro_handle;

                ice::native_file::FileRequestStatus const status = ice::native_file::write_file_request(
                    *this, _file, _offset.to_usize(), _data
                );
                ICE_ASSERT_CORE(status != ice::native_file::FileRequestStatus::Error);
                return status != ice::native_file::FileRequestStatus::Completed;
            }

            inline bool await_resume() const noexcept { return true; }
        };

    } // namespace detail

    WritableFileResource::WritableFileResource(
        ice::Allocator& alloc,
        ice::usize meta_size,
        ice::usize data_size,
        ice::HeapString<> origin_path,
        ice::String origin_name,
        ice::String uri_path
    ) noexcept
        : ice::WritableFileSystemResource{ }
        , _allocator{ alloc }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri_path{ uri_path }
        , _uri{ ice::Scheme_File, uri_path }
        , _metasize{ meta_size }
        , _datasize{ data_size }
    {
    }

    WritableFileResource::~WritableFileResource() noexcept
    {
    }

    auto WritableFileResource::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto WritableFileResource::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::None;
    }

    auto WritableFileResource::name() const noexcept -> ice::String
    {
        return _origin_name;
    }

    auto WritableFileResource::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto WritableFileResource::load_named_part(
        ice::StringID_Arg name,
        ice::Allocator& alloc
    ) const noexcept -> ice::Task<Memory>
    {
        ICE_ASSERT_CORE(false);
        co_return ice::Memory{};
    }

    void WritableFileResource::add_named_part(
        ice::StringID_Arg name,
        ice::HeapString<> path
    ) noexcept
    {
        ICE_ASSERT_CORE(false);
    }

    auto WritableFileResource::load_data(
        ice::Allocator& alloc,
        ice::Memory& memory,
        ice::String fragment,
        ice::native_aio::AIOPort aioport
    ) const noexcept -> ice::TaskExpected<ice::Data>
    {
        ICE_ASSERT(
            memory.location == nullptr || memory.size >= (_datasize + _metasize),
            "Allocated memory is not large enough to store resource data and metadata!"
        );

        ice::usize const metaoffset = ice::align_to(_datasize, ice::ualign::b_8).value;
        if (memory.location == nullptr)
        {
            memory = alloc.allocate(metaoffset + _metasize);

            ice::Memory targetmem = memory;
            if (aioport != nullptr)
            {
                targetmem.size = _datasize;
                co_await detail::async_file_load(alloc, targetmem, aioport, _origin_path, false);
                targetmem = ice::ptr_add(memory, metaoffset);
                targetmem.size = _metasize;
                co_await detail::async_file_load(alloc, targetmem, aioport, _origin_path, true);
            }
            else
            {
                targetmem.size = _datasize;
                detail::sync_file_load(alloc, targetmem, _origin_path, false);
                targetmem = ice::ptr_add(memory, metaoffset);
                targetmem.size = _metasize;
                detail::sync_file_load(alloc, targetmem, _origin_path, true);
            }
        }

        if (fragment == "meta")
        {
            co_return Data{ ice::ptr_add(memory.location, metaoffset), _metasize, ice::ualign::b_8 };
        }
        else
        {
            co_return Data{ memory.location, _datasize, ice::ualign::b_default };
        }
    }

    auto WritableFileResource::size() const noexcept -> ice::usize
    {
        ice::StackAllocator_1024 alloc;
        ice::native_file::HeapFilePath path{ alloc };
        ice::native_file::path_from_string(path, _origin_path);
        return ice::native_file::sizeof_file(path);
    }

    auto WritableFileResource::write_data(
        ice::Allocator& alloc,
        ice::Data data,
        ice::usize write_offset,
        ice::String fragment,
        ice::native_aio::AIOPort aioport
    ) const noexcept -> ice::TaskExpected<ice::usize>
    {
        using enum ice::native_file::FileOpenFlags;

        ice::native_file::HeapFilePath native_filepath{ alloc };
        ice::native_file::path_from_string(native_filepath, _origin_path);

        ice::Expected result = ice::native_file::open_file(aioport, native_filepath, Write | Asynchronous);
        if (result.failed())
        {
            co_return result.error();
        }

        ice::native_file::File file = ice::move(result).value();
        ice::detail::WriterAwaitable awaitable{ aioport, file, (ice::isize) write_offset, data };
        co_await awaitable;
        co_return data.size;
    }

    auto create_writable_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_filepath,
        ice::native_file::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        IPT_ZONE_SCOPED;
        ice::WritableFileResource* main_resource = nullptr;
        ice::usize const meta_size = ice::native_file::sizeof_file(meta_filepath);
        ice::usize const data_size = ice::native_file::sizeof_file(data_filepath);

        // We create the main resource in a different scope so we dont accidentaly use data from there
        {
            ice::HeapString<> utf8_file_path{ alloc };
            ice::native_file::path_to_string(data_filepath, utf8_file_path);
            ice::path::normalize(utf8_file_path);
            IPT_ZONE_TEXT_STR(utf8_file_path);

            // TODO: Decide how to handle the basepath naming.
            bool const remove_slash = utf8_file_path[ice::path::length(base_path)] == '/';
            ice::String utf8_origin_name = ice::string::substr(utf8_file_path, ice::path::length(base_path) + remove_slash);
            ice::String utf8_uri_path = ice::string::substr(utf8_file_path, ice::path::length(uri_base_path));

            IPT_ZONE_SCOPED_NAMED("stage: create_writable_resource");
            main_resource = ice::create_resource_object<ice::WritableFileResource>(
                alloc,
                provider,
                alloc,
                meta_size,
                data_size,
                ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                utf8_origin_name,
                utf8_uri_path
            );
        }

        return main_resource;
    }

} // namespace ice
