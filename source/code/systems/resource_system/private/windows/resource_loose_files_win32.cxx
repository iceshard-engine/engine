/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_loose_files_win32.hxx"
#include <ice/mem_allocator_stack.hxx>
#include <ice/container_types.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string_utils.hxx>
#include <ice/assert.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/profiler.hxx>

#include "resource_utils_win32.hxx"
#include "resource_asyncio_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    auto internal_overlapped_file_load(
        ice::win32::FileHandle file,
        ice::ucount filesize,
        ice::NativeIO* nativeio,
        ice::Memory data
    ) noexcept -> ice::Task<bool>
    {
        ice::AsyncIOData asyncio{ nativeio, ice::move(file), data, filesize };
        AsyncIOData::Result read_result = co_await asyncio;
        co_return read_result.success && read_result.bytes_read >= filesize;
    }

    auto internal_file_load(
        ice::Allocator& alloc,
        ice::NativeIO* nativeio,
        ice::String filepath
    ) noexcept -> ice::Task<ice::Memory>
    {
        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::utf8_to_wide_append(filepath, file_location_wide);

        ice::Memory result{
            .location = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid,
        };

        ice::win32::FileHandle file_handle = ice::win32::native_open_file(
            file_location_wide,
            FILE_FLAG_OVERLAPPED
        );
        if (file_handle)
        {
            LARGE_INTEGER file_size;
            if (GetFileSizeEx(file_handle.native(), &file_size) == 0)
            {
                co_return result;
            }

            ice::Memory const data = alloc.allocate(
                AllocRequest{ { ice::usize::base_type(file_size.QuadPart) }, ice::ualign::b_default }
            );

            bool const success = co_await internal_overlapped_file_load(ice::move(file_handle), (ice::ucount)file_size.QuadPart, nativeio, data);
            if (success)
            {
                result = data;
            }
            else
            {
                alloc.deallocate(data);
            }
        }

        co_return result;
    }


    Resource_LooseFilesWin32::Resource_LooseFilesWin32(
        ice::Allocator& alloc,
        ice::MutableMetadata metadata,
        ice::HeapString<> origin_path,
        ice::String origin_name,
        ice::String uri_path
    ) noexcept
        : ice::Resource_Win32{ }
        , _mutable_metadata{ ice::move(metadata) }
        , _metadata{ _mutable_metadata }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri_path{ uri_path }
        , _uri{ ice::Scheme_File, uri_path }
        , _extra_resources{ alloc }
    {
    }

    Resource_LooseFilesWin32::~Resource_LooseFilesWin32() noexcept
    {
    }

    auto Resource_LooseFilesWin32::uri() const noexcept -> ice::URI const&
    {
        return _uri;
    }

    auto Resource_LooseFilesWin32::flags() const noexcept -> ice::ResourceFlags
    {
        return ice::ResourceFlags::None;
    }

    auto Resource_LooseFilesWin32::name() const noexcept -> ice::String
    {
        return _origin_name;
    }

    auto Resource_LooseFilesWin32::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto Resource_LooseFilesWin32::metadata() const noexcept -> ice::Metadata const&
    {
        return _metadata;
    }

    auto Resource_LooseFilesWin32::load_named_part(
        ice::StringID_Arg name,
        ice::Allocator& alloc
    ) const noexcept -> ice::Task<Memory>
    {
        ice::Memory result{
            .location = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid,
        };

        if (ice::hashmap::has(_extra_resources, ice::hash(name)))
        {
            ice::HeapString<> empty_str{ alloc };
            ice::String path = ice::hashmap::get(_extra_resources, ice::hash(name), empty_str);

            ice::HeapString<wchar_t> file_location_wide{ alloc };
            ice::utf8_to_wide_append(path, file_location_wide);

            ice::win32::FileHandle const file_handle = ice::win32::native_open_file(
                file_location_wide,
                FILE_ATTRIBUTE_NORMAL
            );
            if (file_handle)
            {
                LARGE_INTEGER file_size;
                if (GetFileSizeEx(file_handle.native(), &file_size) == 0)
                {
                    co_return result;
                }

                if (ice::win32::native_load_file(file_handle, alloc, result) == false)
                {
                    ICE_LOG(ice::LogSeverity::Error, ice::LogTag::System, "Failed to load resource part {}", name);
                }
            }
        }
        co_return result;
    }

    void Resource_LooseFilesWin32::add_named_part(
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

    auto Resource_LooseFilesWin32::load_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::NativeIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        co_return co_await internal_file_load(alloc, nativeio, _origin_path);
    }

    Resource_LooseFilesWin32::ExtraResource::ExtraResource(
        ice::Resource_LooseFilesWin32& parent,
        ice::HeapString<> origin_path,
        ice::ResourceFlags flags
    ) noexcept
        : _parent{ parent }
        , _origin_path{ ice::move(origin_path) }
        , _flags{ flags }
    {
    }

    auto Resource_LooseFilesWin32::ExtraResource::uri() const noexcept -> ice::URI const&
    {
        return _parent.uri();
    }

    auto Resource_LooseFilesWin32::ExtraResource::flags() const noexcept -> ice::ResourceFlags
    {
        return _flags;
    }

    auto Resource_LooseFilesWin32::ExtraResource::name() const noexcept -> ice::String
    {
        return _parent.name();
    }

    auto Resource_LooseFilesWin32::ExtraResource::origin() const noexcept -> ice::String
    {
        return _origin_path;
    }

    auto Resource_LooseFilesWin32::ExtraResource::metadata() const noexcept -> ice::Metadata const&
    {
        return _parent.metadata();
    }

    auto Resource_LooseFilesWin32::ExtraResource::load_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& scheduler,
        ice::NativeIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        co_return co_await internal_file_load(alloc, nativeio, _origin_path);
    }

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString uri_base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_Win32*
    {
        ice::Resource_LooseFilesWin32* main_resource = nullptr;
        ice::win32::FileHandle meta_handle = ice::win32::native_open_file(meta_file, FILE_ATTRIBUTE_NORMAL);
        ice::win32::FileHandle data_handle = ice::win32::native_open_file(data_file, FILE_ATTRIBUTE_NORMAL);

        if (meta_handle && data_handle)
        {
            ice::Memory metafile_data;
            if (ice::win32::native_load_file(meta_handle, alloc, metafile_data) && (metafile_data.size.value > ice::string::size(Constant_FileHeader_MetadataFile)))
            {
                ice::String const file_header{
                    reinterpret_cast<char const*>(metafile_data.location),
                    ice::string::size(Constant_FileHeader_MetadataFile)
                };

                if (Constant_FileHeader_MetadataFile == file_header)
                {
                    return main_resource;
                }

                // We create the main resource in a different scope so we dont accidentaly use data from there
                {
                    ice::HeapString<> utf8_file_path{ alloc };
                    ice::wide_to_utf8_append(data_file, utf8_file_path);
                    ice::path::normalize(utf8_file_path);

                    ice::String utf8_origin_name = ice::string::substr(utf8_file_path, ice::wide_to_utf8_size(base_path));
                    ice::String utf8_uri_path = ice::string::substr(utf8_file_path, ice::wide_to_utf8_size(uri_base_path));

                    // We have a loose resource files which contain metadata associated data.
                    // We need now to read the metadata and check if there are more file associated and if all are available.
                    ice::MutableMetadata mutable_meta{ alloc };
                    ice::meta_deserialize(data_view(metafile_data), mutable_meta);
                    alloc.deallocate(metafile_data);

                    main_resource = alloc.create<ice::Resource_LooseFilesWin32>(
                        alloc,
                        ice::move(mutable_meta),
                        ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                        utf8_origin_name,
                        utf8_uri_path
                    );
                }

                // We can access the metadata now again.
                ice::Metadata const& metadata = main_resource->metadata();

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

                    // Lets take a bet that we can use at least 20 more characters before growing!
                    ice::u32 const expected_extra_path_size = ice::string::size(meta_file) + 16;
                    ice::string::reserve(utf8_file_path, expected_extra_path_size);

                    // Create a wide string to try and check for extra files existing.
                    ice::HeapString<wchar_t> full_path{ alloc };
                    ice::string::reserve(full_path, ice::string::capacity(utf8_file_path));

                    full_path = ice::path::win32::directory(meta_file);
                    ice::string::push_back(full_path, L'/');

                    // Remember the size so we can quickly resize.
                    ice::u32 const base_dir_size = ice::string::size(full_path);

                    for (ice::u32 idx = 0; idx < ice::array::count(paths); ++idx)
                    {
                        ICE_ASSERT(
                            ice::string::any(names[idx]),
                            "Extra files need to be declared with specific names!"
                        );

                        ice::string::resize(full_path, base_dir_size);
                        ice::utf8_to_wide_append(paths[idx], full_path);

                        ice::win32::FileHandle extra_handle = ice::win32::native_open_file(full_path, FILE_ATTRIBUTE_NORMAL);
                        if (extra_handle)
                        {
                            // We know the file can be opened so we save it as a extra resource.
                            ice::wide_to_utf8_append(full_path, utf8_file_path);
                            ice::path::normalize(utf8_file_path);

                            main_resource->add_named_part(ice::stringid(names[idx]), ice::move(utf8_file_path));
                        }
                    }
                }
            }
        }

        return main_resource;
    }


} // namespace ice

#endif // #if ISP_WINDOWS
