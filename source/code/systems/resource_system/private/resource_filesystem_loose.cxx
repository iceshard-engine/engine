/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_loose.hxx"
#include "native/native_aio_tasks.hxx"

#include <ice/mem_allocator_stack.hxx>
#include <ice/path_utils.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>

namespace ice
{

    namespace detail
    {

        auto async_file_read(
            ice::native_file::File file,
            ice::usize filesize,
            ice::NativeAIO* nativeio,
            ice::Memory data
        ) noexcept -> ice::Task<bool>
        {
            ice::AsyncReadFile asyncio{ nativeio, ice::move(file), data, filesize };
            ice::AsyncReadFile::Result const read_result = co_await asyncio;
            ICE_ASSERT(
                read_result.bytes_read == filesize,
                "Failed to load file into memory, requested bytes: {}!",
                filesize
            );
            co_return read_result.bytes_read == filesize;
        }

        auto async_file_load(
            ice::Allocator& alloc,
            ice::NativeAIO* nativeio,
            ice::String filepath
        ) noexcept -> ice::Task<ice::Memory>
        {
            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_file::File handle = ice::native_file::open_file(
                native_filepath,
                ice::native_file::FileOpenFlags::Asynchronous
            );
            if (handle)
            {
                ice::usize const filesize = ice::native_file::sizeof_file(handle);
                ICE_ASSERT(
                    filesize.value < ice::ucount_max,
                    "Trying to load file larger than supported!"
                );

                if (filesize > 0_B)
                {
                    result = alloc.allocate(filesize);
                    bool const success = co_await async_file_read(ice::move(handle), filesize, nativeio, result);
                    if (success == false)
                    {
                        alloc.deallocate(result);
                    }
                }
                else
                {
                    // Allocate 1 byte just to not return a nullptr.
                    // TODO: Find a better way to handle this.
                    result = alloc.allocate(1_B);
                }
            }
            co_return result;
        }

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::String filepath
        ) noexcept -> ice::Memory
        {
            ice::native_file::HeapFilePath native_filepath{ alloc };
            ice::native_file::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_file::File handle = ice::native_file::open_file(native_filepath);
            if (handle)
            {
                ice::usize const filesize = ice::native_file::sizeof_file(handle);
                ICE_ASSERT(
                    filesize.value < ice::ucount_max,
                    "Trying to load file larger than supported!"
                );

                result = alloc.allocate(filesize);
                // TODO: Better error handling. Using "expected".
                ice::usize const bytes_read = ice::native_file::read_file(
                    ice::move(handle),
                    filesize,
                    result
                );
                if (bytes_read.value == 0)
                {
                    ICE_ASSERT(
                        bytes_read.value != 0,
                        "Failed to load file!"
                    );

                    alloc.deallocate(result);
                }
            }
            return result;
        }

        bool load_metadata_file(
            ice::Allocator& alloc,
            ice::native_file::FilePath path,
            ice::Memory& out_memory,
            ice::MutableMetadata& out_metadata
        ) noexcept
        {
            ice::native_file::File meta_handle = ice::native_file::open_file(path);
            if (meta_handle == false)
            {
                return false;
            }

            ice::usize const meta_size = ice::native_file::sizeof_file(meta_handle);
            if (meta_size.value <= ice::string::size(Constant_FileHeader_MetadataFile))
            {
                return false;
            }

            ice::Memory metafile_data = alloc.allocate(meta_size);
            if (ice::native_file::read_file(meta_handle, meta_size, metafile_data) > 0_B)
            {
                ice::Result const result = ice::meta_deserialize_from(out_metadata, data_view(metafile_data));
                if (ice::result_is_valid(result))
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
        ice::Memory metadata,
        ice::HeapString<> origin_path,
        ice::String origin_name,
        ice::String uri_path
    ) noexcept
        : ice::FileSystemResource{ }
        , _allocator{ alloc }
        , _raw_metadata{ metadata }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri_path{ uri_path }
        , _uri{ ice::Scheme_File, uri_path }
        , _extra_resources{ alloc }
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

    auto LooseFilesResource::load_metadata() const noexcept -> ice::Task<ice::Data>
    {
        co_return ice::data_view(_raw_metadata);
    }

    auto LooseFilesResource::load_named_part(
        ice::StringID_Arg name,
        ice::Allocator& alloc
    ) const noexcept -> ice::Task<Memory>
    {
        ice::Memory result{
            .location = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid,
        };

        ice::HeapString<> const* const extra_path = ice::hashmap::try_get(_extra_resources, ice::hash(name));
        if (extra_path != nullptr)
        {
            result = ice::detail::sync_file_load(alloc, *extra_path);
        }
        co_return result;
    }

    void LooseFilesResource::add_named_part(
        ice::StringID_Arg name,
        ice::HeapString<> path
    ) noexcept
    {
        ICE_ASSERT(
            ice::hashmap::has(_extra_resources, ice::hash(name)) == false,
            "Extra file with name {} already exists!", name
        );

        ice::hashmap::set(_extra_resources, ice::hash(name), ice::move(path));
    }

    auto LooseFilesResource::load_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::NativeAIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        if (nativeio != nullptr)
        {
            co_return co_await detail::async_file_load(alloc, nativeio, _origin_path);
        }
        else
        {
            co_return detail::sync_file_load(alloc, _origin_path);
        }
    }

    auto LooseFilesResource::size() const noexcept -> ice::usize
    {
        ice::StackAllocator_1024 alloc;
        ice::native_file::HeapFilePath path{ alloc };
        ice::native_file::path_from_string(_origin_path, path);
        return ice::native_file::sizeof_file(path);
    }

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
        ice::TaskScheduler& scheduler,
        ice::NativeAIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        co_return co_await detail::async_file_load(alloc, nativeio, _origin_path);
    }

    auto LooseFilesResource::ExtraResource::size() const noexcept -> ice::usize
    {
        ice::StackAllocator_1024 alloc;
        ice::native_file::HeapFilePath path{ alloc };
        ice::native_file::path_from_string(_origin_path, path);
        return ice::native_file::sizeof_file(path);
    }

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_filepath,
        ice::native_file::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        IPT_ZONE_SCOPED;
        ice::LooseFilesResource* main_resource = nullptr;
        bool const data_exists = true;// ice::native_file::exists_file(data_filepath);
        bool const meta_exists = ice::native_file::exists_file(meta_filepath);

        ice::MutableMetadata mutable_meta{ alloc };
        if (data_exists)
        {
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

                // We have a loose resource files which contain metadata associated data.
                // We need now to read the metadata and check if there are more file associated and if all are available.
                ice::Memory raw_metadata{};
                if (meta_exists)
                {
                    IPT_ZONE_SCOPED_NAMED("stage: read_metadata");
                    bool meta_load_success = ice::detail::load_metadata_file(alloc, meta_filepath, raw_metadata, mutable_meta);
                    ICE_LOG_IF(
                        meta_load_success == false,
                        LogSeverity::Warning, LogTag::Core,
                        "Failed to load metadata file {}.isrm",
                        (ice::String)utf8_file_path
                    );
                }

                IPT_ZONE_SCOPED_NAMED("stage: create_resource");
                main_resource = alloc.create<ice::LooseFilesResource>(
                    alloc,
                    raw_metadata,
                    ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                    utf8_origin_name,
                    utf8_uri_path
                );
            }

            IPT_ZONE_SCOPED_NAMED("stage: extra_files");
            ice::Metadata const metadata = mutable_meta;
            bool const has_paths = ice::meta_has_entry(metadata, "resource.extra_files.paths"_sid);
            bool const has_names = ice::meta_has_entry(metadata, "resource.extra_files.names"_sid);
            ICE_ASSERT(
                (has_paths ^ has_names) == 0,
                "If a resource defines extra files, it's only allowed to do so by providing 'unique flags' and co-related 'relative paths'."
            );

            ice::Array<ice::String> paths{ alloc };
            ice::array::reserve(paths, 4);

            ice::Array<ice::String> names{ alloc };
            ice::array::reserve(names, 4);

            if (has_paths && has_names)
            {
                ice::meta_read_string_array(metadata, "resource.extra_files.paths"_sid, paths);
                ice::meta_read_string_array(metadata, "resource.extra_files.names"_sid, names);

                ice::HeapString<> utf8_file_path{ alloc };

                // Lets take a bet that we can use at least 32 more characters before growing!
                ice::u32 const expected_extra_path_size = ice::string::size(meta_filepath) + 32;
                ice::string::reserve(utf8_file_path, expected_extra_path_size);

                // Create a wide string to try and check for extra files existing.
                ice::native_file::HeapFilePath full_path{ alloc };
                ice::string::reserve(full_path, ice::string::capacity(utf8_file_path));
                full_path = meta_filepath;

                // Remember the size so we can quickly resize.
                ice::u32 const base_dir_size = ice::string::size(full_path);

                for (ice::u32 idx = 0; idx < ice::array::count(paths); ++idx)
                {
                    ICE_ASSERT(
                        ice::string::any(names[idx]),
                        "Extra files need to be declared with specific names!"
                    );

                    // TODO:
                    ice::string::resize(full_path, base_dir_size);
                    ice::native_file::path_join_string(full_path, paths[idx]);
                    if (ice::native_file::exists_file(full_path))
                    {
                        // We know the file can be opened so we save it as a extra resource.
                        ice::native_file::path_to_string(full_path, utf8_file_path);
                        ice::path::normalize(utf8_file_path);

                        main_resource->add_named_part(ice::stringid(names[idx]), ice::move(utf8_file_path));
                    }
                }
            }
        }

        // return main_resource;
        return main_resource;
    }

} // namespace ice
