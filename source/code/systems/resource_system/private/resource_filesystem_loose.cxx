/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_filesystem_loose.hxx"
#include "native/native_aio_tasks.hxx"

#include <ice/path_utils.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/task.hxx>
#include <ice/task_utils.hxx>

namespace ice
{

    namespace detail
    {

        auto async_file_read(
            ice::native_fileio::File file,
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
            ice::native_fileio::HeapFilePath native_filepath{ alloc };
            ice::native_fileio::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_fileio::File handle = ice::native_fileio::open_file(
                native_filepath,
                ice::native_fileio::FileOpenFlags::Asynchronous
            );
            if (handle)
            {
                ice::usize const filesize = ice::native_fileio::sizeof_file(handle);
                ICE_ASSERT(
                    filesize.value < ice::ucount_max,
                    "Trying to load file larger than supported!"
                );

                result = alloc.allocate(filesize);
                bool const success = co_await async_file_read(ice::move(handle), filesize, nativeio, result);
                if (success == false)
                {
                    alloc.deallocate(result);
                }
            }
            co_return result;
        }

        auto sync_file_load(
            ice::Allocator& alloc,
            ice::String filepath
        ) noexcept -> ice::Memory
        {
            ice::native_fileio::HeapFilePath native_filepath{ alloc };
            ice::native_fileio::path_from_string(filepath, native_filepath);

            ice::Memory result{
                .location = nullptr,
                .size = 0_B,
                .alignment = ice::ualign::invalid,
            };

            ice::native_fileio::File handle = ice::native_fileio::open_file(native_filepath);
            if (handle)
            {
                ice::usize const filesize = ice::native_fileio::sizeof_file(handle);
                ICE_ASSERT(
                    filesize.value < ice::ucount_max,
                    "Trying to load file larger than supported!"
                );

                result = alloc.allocate(filesize);
                // TODO: Better error handling. Using "expected".
                ice::usize const bytes_read = ice::native_fileio::read_file(
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
            ice::native_fileio::FilePath path,
            ice::MutableMetadata& out_metadata
        ) noexcept
        {
            ice::native_fileio::File meta_handle = ice::native_fileio::open_file(path);
            if (meta_handle == false)
            {
                return false;
            }

            ice::usize const meta_size = ice::native_fileio::sizeof_file(meta_handle);
            if (meta_size.value <= ice::string::size(Constant_FileHeader_MetadataFile))
            {
                return false;
            }

            ice::Memory metafile_data = alloc.allocate(meta_size);
            if (ice::native_fileio::read_file(meta_handle, meta_size, metafile_data) == 0_B)
            {
                return false;
            }

            ice::String const file_header{
                reinterpret_cast<char const*>(metafile_data.location),
                ice::string::size(Constant_FileHeader_MetadataFile)
            };

            // Don't create resources from meta-files
            if (Constant_FileHeader_MetadataFile == file_header)
            {
                return false;
            }

            ice::Result const result = ice::meta_deserialize_from(out_metadata, data_view(metafile_data));
            alloc.deallocate(metafile_data);
            return ice::result_is_valid(result);
        }

    } // namespace detail

    LooseFilesResource::LooseFilesResource(
        ice::Allocator& alloc,
        ice::MutableMetadata metadata,
        ice::HeapString<> origin_path,
        ice::String origin_name,
        ice::String uri_path
    ) noexcept
        : ice::FileSystemResource{ }
        , _mutable_metadata{ ice::move(metadata) }
        , _metadata{ _mutable_metadata }
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

    auto LooseFilesResource::load_metadata(ice::Metadata& out_metadata) const noexcept -> ice::Task<bool>
    {
        out_metadata = _metadata;
        co_return true;
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

    auto LooseFilesResource::ExtraResource::load_metadata(ice::Metadata& out_metadata) const noexcept -> ice::Task<bool>
    {
        return _parent.load_metadata(out_metadata);
    }

    auto LooseFilesResource::ExtraResource::load_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::NativeAIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        co_return co_await detail::async_file_load(alloc, nativeio, _origin_path);
    }

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::native_fileio::FilePath base_path,
        ice::native_fileio::FilePath uri_base_path,
        ice::native_fileio::FilePath meta_filepath,
        ice::native_fileio::FilePath data_filepath
    ) noexcept -> ice::FileSystemResource*
    {
        ice::LooseFilesResource* main_resource = nullptr;
        bool const data_exists = ice::native_fileio::exists_file(data_filepath);
        bool const meta_exists = ice::native_fileio::exists_file(meta_filepath);

        if (data_exists)
        {
            // We create the main resource in a different scope so we dont accidentaly use data from there
            {
                ice::HeapString<> utf8_file_path{ alloc };
                ice::native_fileio::path_to_string(data_filepath, utf8_file_path);
                ice::path::normalize(utf8_file_path);

                // TODO: Decide how to handle the basepath naming.
                bool const remove_slash = utf8_file_path[ice::path::length(base_path)] == '/';
                ice::String utf8_origin_name = ice::string::substr(utf8_file_path, ice::path::length(base_path) + remove_slash);
                ice::String utf8_uri_path = ice::string::substr(utf8_file_path, ice::path::length(uri_base_path));

                // We have a loose resource files which contain metadata associated data.
                // We need now to read the metadata and check if there are more file associated and if all are available.
                ice::MutableMetadata mutable_meta{ alloc };
                if (meta_exists)
                {
                    bool meta_load_success = ice::detail::load_metadata_file(alloc, meta_filepath, mutable_meta);
                    ICE_LOG_IF(
                        meta_load_success == false,
                        LogSeverity::Warning, LogTag::Core,
                        "Failed to load metadata file {}.isrm",
                        (ice::String)utf8_file_path
                    );
                }

                main_resource = alloc.create<ice::LooseFilesResource>(
                    alloc,
                    ice::move(mutable_meta),
                    ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                    utf8_origin_name,
                    utf8_uri_path
                );
            }

            // We can access the metadata now again.
            ice::Metadata metadata;
            bool const success = ice::wait_for(main_resource->load_metadata(metadata));
            ICE_ASSERT_CORE(success);

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
                ice::native_fileio::HeapFilePath full_path{ alloc };
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
                    ice::native_fileio::path_join_string(full_path, paths[idx]);
                    if (ice::native_fileio::exists_file(full_path))
                    {
                        // We know the file can be opened so we save it as a extra resource.
                        ice::native_fileio::path_to_string(full_path, utf8_file_path);
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
