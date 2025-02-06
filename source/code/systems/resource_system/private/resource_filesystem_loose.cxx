/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_loose.hxx"
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
        ) noexcept -> ice::Task<bool>
        {
            ice::detail::AsyncReadRequest request{ aioport, file, filesize, 0_B, data };
            ice::usize const bytes_read = co_await request;

            ICE_ASSERT(
                bytes_read == filesize,
                "Failed to load file into memory, requested bytes: {}!",
                filesize
            );
            co_return bytes_read == filesize;
        }

        auto async_file_load(
            ice::Allocator& alloc,
            ice::Memory target_memory,
            ice::native_aio::AIOPort aioport,
            ice::String filepath,
            bool readmeta
        ) noexcept -> ice::Task<ice::Result>
        {
            if (target_memory.size == 0_B)
            {
                co_return S_Ok;
            }

            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(native_filepath, filepath);
            if (readmeta)
            {
                ice::string::push_back(native_filepath, ISP_PATH_LITERAL(".isrm"));
            }

            ice::Expected<ice::native_file::File> handle = ice::native_file::open_file(aioport, native_filepath);
            if (handle)
            {
                co_return co_await async_file_read(ice::move(handle).value(), target_memory.size, aioport, target_memory)
                    ? S_Ok : E_Fail;
            }
            co_return handle.error();
        }

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::Memory target_memory,
            ice::String filepath,
            bool readmeta
        ) noexcept -> ice::Result
        {
            if (target_memory.size == 0_B)
            {
                return S_Ok;
            }

            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(native_filepath, filepath);
            if (readmeta)
            {
                ice::string::push_back(native_filepath, ISP_PATH_LITERAL(".isrm"));
            }

            ice::native_file::File handle = ice::native_file::open_file(native_filepath);
            if (handle == false)
            {
                return E_Fail;
            }

            ice::usize const bytes_read = ice::native_file::read_file(
                ice::move(handle),
                target_memory.size,
                target_memory
            );
            return bytes_read == 0_B;
        }

        bool load_metadata_file(
            ice::Allocator& alloc,
            ice::native_file::FilePath path,
            ice::Memory& out_memory,
            ice::ConfigBuilder& out_metadata
        ) noexcept
        {
            ice::native_file::File meta_handle = ice::native_file::open_file(path);
            if (meta_handle == false)
            {
                return false;
            }

            ice::usize const meta_size = ice::native_file::sizeof_file(meta_handle);

            ice::Memory metafile_data = alloc.allocate(meta_size);
            if (ice::native_file::read_file(meta_handle, meta_size, metafile_data) > 0_B)
            {
                if (ice::config::from_json(out_metadata, ice::string::from_data(metafile_data)))
                {
                    // return the memory, we won't release it
                    out_memory = metafile_data;
                    return true;
                }
            }

            alloc.deallocate(metafile_data);
            return false;
        }

    } // namespace detail

    LooseFilesResource::LooseFilesResource(
        ice::Allocator& alloc,
        ice::usize meta_size,
        ice::usize data_size,
        ice::HeapString<> origin_path,
        ice::String origin_name,
        ice::String uri_path
    ) noexcept
        : ice::FileSystemResource{ }
        , _allocator{ alloc }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri_path{ uri_path }
        , _uri{ ice::Scheme_File, uri_path }
        , _metasize{ meta_size }
        , _datasize{ data_size }
    {
    }

    LooseFilesResource::~LooseFilesResource() noexcept
    {
    }

    auto LooseFilesResource::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto LooseFilesResource::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::None;
    }

    auto LooseFilesResource::name() const noexcept -> ice::String
    {
        return _origin_name;
    }

    auto LooseFilesResource::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto LooseFilesResource::load_named_part(
        ice::StringID_Arg name,
        ice::Allocator& alloc
    ) const noexcept -> ice::Task<Memory>
    {
        ICE_ASSERT_CORE(false);
        co_return ice::Memory{};
    }

    void LooseFilesResource::add_named_part(
        ice::StringID_Arg name,
        ice::HeapString<> path
    ) noexcept
    {
        ICE_ASSERT_CORE(false);
    }

    auto LooseFilesResource::load_data(
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

    auto LooseFilesResource::size() const noexcept -> ice::usize
    {
        ice::StackAllocator_1024 alloc;
        ice::native_file::HeapFilePath path{ alloc };
        ice::native_file::path_from_string(path, _origin_path);
        return ice::native_file::sizeof_file(path);
    }

#if 0
    LooseFilesResource::ExtraResource::ExtraResource(
        ice::LooseFilesResource& parent,
        ice::HeapString<> origin_path,
        ice::ResourceFlags flags
    ) noexcept
        : _parent{ parent }
        , _origin_path{ ice::move(origin_path) }
        , _flags{ flags }
    {
    }

    auto LooseFilesResource::ExtraResource::uri() const noexcept -> ice::URI const&
    {
        return _parent.uri();
    }

    auto LooseFilesResource::ExtraResource::flags() const noexcept -> ice::ResourceFlags
    {
        return _flags;
    }

    auto LooseFilesResource::ExtraResource::name() const noexcept -> ice::String
    {
        return _parent.name();
    }

    auto LooseFilesResource::ExtraResource::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto LooseFilesResource::ExtraResource::load_metadata() const noexcept -> ice::Task<ice::Data>
    {
        return _parent.load_metadata();
    }

    auto LooseFilesResource::ExtraResource::load_data(
        ice::Allocator& alloc,
        ice::native_aio::AIOPort aioport
    ) const noexcept -> ice::Task<ice::Memory>
    {
        co_return co_await detail::async_file_load(alloc, aioport, _origin_path);
    }

    auto LooseFilesResource::ExtraResource::size() const noexcept -> ice::usize
    {
        ice::StackAllocator_1024 alloc;
        ice::native_file::HeapFilePath path{ alloc };
        ice::native_file::path_from_string(path, _origin_path);
        return ice::native_file::sizeof_file(path);
    }
#endif

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_filepath,
        ice::native_file::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        IPT_ZONE_SCOPED;
        ice::LooseFilesResource* main_resource = nullptr;
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

            IPT_ZONE_SCOPED_NAMED("stage: create_resource");
            main_resource = ice::create_resource_object<ice::LooseFilesResource>(
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
