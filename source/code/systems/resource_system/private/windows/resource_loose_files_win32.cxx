#include "resource_loose_files_win32.hxx"
#include <ice/mem_allocator_stack.hxx>
#include <ice/container_types.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/assert.hxx>

#include "resource_utils_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    void internal_overlapped_completion_routine(
        DWORD dwErrorCode,
        DWORD dwNumberOfBytesTransfered,
        LPOVERLAPPED lpOverlapped
    ) noexcept
    {
        ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::System, "internal_overlapped_completion_routine()!");
    }

    auto internal_overlapped_file_load(HANDLE file, ice::Memory const& data, ice::TaskScheduler_v2& scheduler) noexcept -> ice::Task<bool>
    {
        OVERLAPPED overlapped{ };

        ice::usize file_offset = 0_B;
        ice::usize file_size_remaining = data.size;
        while (file_size_remaining > 0_B)
        {
            overlapped.OffsetHigh = ice::ucount(file_offset.value >> 32);
            overlapped.Offset = ice::ucount(file_offset.value & 0x0000'0000'ffff'ffff);

            co_await scheduler;

            ice::usize constexpr max_bytes_to_read = 1024_B * 32_B;
            ice::usize const bytes_to_read = ice::min(file_size_remaining, max_bytes_to_read);

            BOOL const read_result = ReadFileEx(
                file,
                ice::ptr_add(data.location, file_offset),
                ice::ucount(bytes_to_read.value),
                &overlapped,
                internal_overlapped_completion_routine
            );

            if (read_result == FALSE)
            {
                break;
            }

            DWORD loaded_size;
            BOOL const overlapped_result = GetOverlappedResultEx(file, &overlapped, &loaded_size, 0, TRUE);
            if (overlapped_result == FALSE)
            {
                DWORD const overlapped_error = GetLastError();
                ICE_LOG(ice::LogSeverity::Error, ice::LogTag::Core, "{}", overlapped_error);
                if (overlapped_error != WAIT_IO_COMPLETION)
                {
                    ICE_LOG(ice::LogSeverity::Error, ice::LogTag::Core, "Overlapped error: {}", overlapped_error);
                }
                else
                {
                    ICE_LOG(ice::LogSeverity::Error, ice::LogTag::Core, "WAIT_IO_COMPLETION");
                }
            }
            else if (loaded_size != 0)
            {
                ICE_ASSERT(
                    loaded_size == bytes_to_read.value
                    || loaded_size == max_bytes_to_read.value,
                    "Loaded unexpected number of bytes during read operation! [got: {}, expected: {} or {}]",
                    loaded_size, bytes_to_read, max_bytes_to_read
                );

                file_size_remaining.value -= bytes_to_read.value;
                file_offset += bytes_to_read; // 4 KiB
            }
        }

        ICE_ASSERT(file_size_remaining == 0_B, "Error!");
        co_return file_size_remaining == 0_B;
    }

    Resource_LooseFilesWin32::Resource_LooseFilesWin32(
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

    auto Resource_LooseFilesWin32::load_data_for_flags(
        ice::Allocator& alloc,
        ice::ResourceFlags flags,
        ice::TaskScheduler_v2& scheduler
    ) const noexcept -> ice::Task<ice::Memory>
    {
        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::win32::utf8_to_wide_append(_origin_path, file_location_wide);

        ice::Memory result{
            .location = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid,
        };

        ice::win32::FileHandle const file_handle = ice::win32::native_open_file(
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

            bool const success = co_await internal_overlapped_file_load(file_handle.native(), data, scheduler);
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

    auto Resource_LooseFilesWin32::ExtraResource::load_data_for_flags(
        ice::Allocator& alloc,
        ice::ResourceFlags flags,
        ice::TaskScheduler_v2& scheduler
    ) const noexcept -> ice::Task<ice::Memory>
    {
        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::win32::utf8_to_wide_append(_origin_path, file_location_wide);

        ice::Memory result{
            .location = nullptr,
            .size = 0_B,
            .alignment = ice::ualign::invalid,
        };

        ice::win32::FileHandle const file_handle = ice::win32::native_open_file(
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

            bool const success = co_await internal_overlapped_file_load(file_handle.native(), data, scheduler);
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

    void create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString uri_base_path,
        ice::WString meta_file,
        ice::WString data_file,
        ice::Array<ice::Resource_Win32*>& out_resources
    ) noexcept
    {
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
                    return;
                }

                // We create the main resource in a different scope so we dont accidentaly use data from there
                ice::Resource_LooseFilesWin32* main_resource;
                {
                    ice::HeapString<> utf8_file_path{ alloc };
                    ice::win32::wide_to_utf8(data_file, utf8_file_path);
                    ice::path::normalize(utf8_file_path);

                    ice::String utf8_origin_name = ice::string::substr(utf8_file_path, ice::win32::wide_to_utf8_size(base_path));
                    ice::String utf8_uri_path = ice::string::substr(utf8_file_path, ice::win32::wide_to_utf8_size(uri_base_path));

                    // We have a loose resource files which contain metadata associated data.
                    // We need now to read the metadata and check if there are more file associated and if all are available.
                    ice::MutableMetadata mutable_meta{ alloc };
                    ice::meta_deserialize(data_view(metafile_data), mutable_meta);

                    main_resource = alloc.create<ice::Resource_LooseFilesWin32>(
                        ice::move(mutable_meta),
                        ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                        utf8_origin_name,
                        utf8_uri_path
                    );

                    ice::array::push_back(out_resources, main_resource);
                }

                // We can access the metadata now again.
                ice::Metadata const& metadata = main_resource->metadata();

                bool const has_paths = ice::meta_has_entry(metadata, "resource.extra_files.paths"_sid);
                bool const has_flags = ice::meta_has_entry(metadata, "resource.extra_files.flags"_sid);
                ICE_ASSERT(
                    (has_paths ^ has_flags) == 0,
                    "If a resource defines extra files, it's only allowed to do so by providing 'unique flags' and co-related 'relative paths'."
                );

                ice::Array<ice::String> paths{ alloc };
                ice::array::reserve(paths, 4);

                ice::Array<ice::ResourceFlags> flags{ alloc };
                ice::array::reserve(flags, 4);

                if (has_paths && has_flags)
                {
                    ice::meta_read_string_array(metadata, "resource.extra_files.paths"_sid, paths);
                    ice::meta_read_flags_array(metadata, "resource.extra_files.flags"_sid, flags);

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
                            flags[idx] != ice::ResourceFlags::None,
                            "Extra files need to be declared with specific flags!"
                        );

                        ice::string::resize(full_path, base_dir_size);
                        ice::win32::utf8_to_wide_append(paths[idx], full_path);

                        ice::win32::FileHandle extra_handle = ice::win32::native_open_file(full_path, FILE_ATTRIBUTE_NORMAL);
                        if (extra_handle)
                        {
                            // We know the file can be opened so we save it as a extra resource.
                            ice::win32::wide_to_utf8(full_path, utf8_file_path);
                            ice::path::normalize(utf8_file_path);

                            ice::Resource_LooseFilesWin32::ExtraResource* const extra_resource = alloc.create<ice::Resource_LooseFilesWin32::ExtraResource>(
                                *main_resource,
                                ice::move(utf8_file_path),
                                flags[idx]
                            );

                            ice::array::push_back(out_resources, extra_resource);
                        }
                    }
                }
            }
        }
    }


} // namespace ice

#endif // #if ISP_WINDOWS
