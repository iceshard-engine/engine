#include "resource_loose_files_win32.hxx"
#include <ice/memory/stack_allocator.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/collections.hxx>
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

    auto internal_overlapped_file_load(HANDLE file, Memory& data, ice::TaskScheduler_v2& scheduler) noexcept -> ice::Task<bool>
    {
        OVERLAPPED overlapped{ };

        ice::u32 file_offset = 0;
        ice::u32 file_size_remaining = data.size;
        while (file_size_remaining > 0)
        {
            overlapped.Offset = file_offset;

            co_await scheduler;

            BOOL const read_result = ReadFileEx(
                file,
                ice::memory::ptr_add(data.location, file_offset),
                ice::min(file_size_remaining, 1024u * 32u),
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
            else
            {
                file_size_remaining -= loaded_size;
                file_offset += loaded_size; // 4 KiB
            }
        }

        ICE_ASSERT(file_size_remaining == 0, "Error!");
        co_return file_size_remaining == 0;
    }

    Resource_LooseFilesWin32::Resource_LooseFilesWin32(
        ice::MutableMetadata metadata,
        ice::HeapString<char8_t> origin_path,
        ice::Utf8String origin_name,
        ice::Utf8String uri_path
    ) noexcept
        : ice::Resource_Win32{ }
        , _mutable_metadata{ ice::move(metadata) }
        , _metadata{ _mutable_metadata }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri_path{ uri_path }
        , _uri{ ice::scheme_file, uri_path }
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

    auto Resource_LooseFilesWin32::name() const noexcept -> ice::Utf8String
    {
        return _origin_name;
    }

    auto Resource_LooseFilesWin32::origin() const noexcept -> ice::Utf8String
    {
        return _origin_path;
    }

    auto Resource_LooseFilesWin32::metadata() const noexcept -> ice::Metadata const&
    {
        return _metadata;
    }

    auto Resource_LooseFilesWin32::load_data_for_flags(
        ice::Allocator& alloc,
        ice::u32 flags,
        ice::TaskScheduler_v2& scheduler
    ) const noexcept -> ice::Task<ice::Memory>
    {
        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::utf8_to_wide_append(_origin_path, file_location_wide);

        ice::Memory result{
            .location = nullptr,
            .size = 0,
            .alignment = 0,
        };

        ice::win32::SHHandle const file_handle = ice::win32_open_file(file_location_wide, FILE_FLAG_OVERLAPPED);
        if (file_handle)
        {
            LARGE_INTEGER file_size;
            if (GetFileSizeEx(file_handle.native(), &file_size) == 0)
            {
                co_return result;
            }

            ice::u32 const casted_file_size = static_cast<ice::u32>(file_size.QuadPart);

            // #TODO: Implement a `ice::size` type that will be used for memory only.
            ICE_ASSERT(casted_file_size == file_size.QuadPart, "Unsupported file size of over 4GB!");

            ice::Memory data{
                .location = alloc.allocate(casted_file_size),
                .size = casted_file_size,
                .alignment = 4
            };


            bool const success = co_await internal_overlapped_file_load(file_handle.native(), data, scheduler);
            if (success)
            {
                result = data;
            }
            else
            {
                alloc.deallocate(data.location);
            }

        }

        co_return result;
    }

    Resource_LooseFilesWin32::ExtraResource::ExtraResource(
        ice::Resource_LooseFilesWin32& parent,
        ice::HeapString<char8_t> origin_path,
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

    auto Resource_LooseFilesWin32::ExtraResource::name() const noexcept -> ice::Utf8String
    {
        return _parent.name();
    }

    auto Resource_LooseFilesWin32::ExtraResource::origin() const noexcept -> ice::Utf8String
    {
        return _origin_path;
    }

    auto Resource_LooseFilesWin32::ExtraResource::metadata() const noexcept -> ice::Metadata const&
    {
        return _parent.metadata();
    }

    auto Resource_LooseFilesWin32::ExtraResource::load_data_for_flags(
        ice::Allocator& alloc,
        ice::u32 flags,
        ice::TaskScheduler_v2& scheduler
    ) const noexcept -> ice::Task<ice::Memory>
    {
        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::utf8_to_wide_append(_origin_path, file_location_wide);

        ice::Memory result{
            .location = nullptr,
            .size = 0,
            .alignment = 0,
        };

        ice::win32::SHHandle const file_handle = ice::win32_open_file(file_location_wide, FILE_FLAG_OVERLAPPED);
        if (file_handle)
        {
            LARGE_INTEGER file_size;
            if (GetFileSizeEx(file_handle.native(), &file_size) == 0)
            {
                co_return result;
            }

            ice::u32 const casted_file_size = static_cast<ice::u32>(file_size.QuadPart);

            // #TODO: Implement a `ice::size` type that will be used for memory only.
            ICE_ASSERT(casted_file_size == file_size.QuadPart, "Unsupported file size of over 4GB!");

            ice::Memory data{
                .location = alloc.allocate(casted_file_size),
                .size = casted_file_size,
                .alignment = 4
            };


            bool const success = co_await internal_overlapped_file_load(file_handle.native(), data, scheduler);
            if (success)
            {
                result = data;
            }
            else
            {
                alloc.deallocate(data.location);
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
        ice::pod::Array<ice::Resource_Win32*>& out_resources
    ) noexcept
    {
        ice::win32::SHHandle meta_handle = win32_open_file(meta_file, FILE_ATTRIBUTE_NORMAL);
        ice::win32::SHHandle data_handle = win32_open_file(data_file, FILE_ATTRIBUTE_NORMAL);

        if (meta_handle && data_handle)
        {
            ice::Buffer meta_file_data{ alloc };

            if (win32_load_file(meta_handle, meta_file_data) && ice::buffer::size(meta_file_data) > ice::string::size(Constant_FileHeader_MetadataFile))
            {
                ice::String const file_header{
                    reinterpret_cast<char const*>(ice::buffer::data(meta_file_data)),
                    ice::string::size(Constant_FileHeader_MetadataFile)
                };

                if (Constant_FileHeader_MetadataFile == file_header)
                {
                    return;
                }

                // We create the main resource in a different scope so we dont accidentaly use data from there
                ice::Resource_LooseFilesWin32* main_resource;
                {
                    ice::HeapString<char8_t> utf8_file_path{ alloc };
                    wide_to_utf8(data_file, utf8_file_path);
                    ice::path::normalize(utf8_file_path);

                    ice::Utf8String utf8_origin_name = ice::string::substr(utf8_file_path, wide_to_utf8_size(base_path));
                    ice::Utf8String utf8_uri_path = ice::string::substr(utf8_file_path, wide_to_utf8_size(uri_base_path));

                    // We have a loose resource files which contain metadata associated data.
                    // We need now to read the metadata and check if there are more file associated and if all are available.
                    ice::MutableMetadata mutable_meta{ alloc };
                    ice::meta_deserialize(meta_file_data, mutable_meta);

                    main_resource = alloc.make<ice::Resource_LooseFilesWin32>(
                        ice::move(mutable_meta),
                        ice::move(utf8_file_path), // we move so the pointer 'origin_name' calculated from 'utf8_file_path' is still valid!
                        utf8_origin_name,
                        utf8_uri_path
                    );

                    ice::pod::array::push_back(out_resources, main_resource);
                }

                // We can access the metadata now again.
                ice::Metadata const& metadata = main_resource->metadata();

                bool const has_paths = ice::meta_has_entry(metadata, "resource.extra_files.paths"_sid);
                bool const has_flags = ice::meta_has_entry(metadata, "resource.extra_files.flags"_sid);
                ICE_ASSERT(
                    (has_paths ^ has_flags) == 0,
                    "If a resource defines extra files, it's only allowed to do so by providing 'unique flags' and co-related 'relative paths'."
                );

                ice::pod::Array<ice::Utf8String> paths{ alloc };
                ice::pod::array::reserve(paths, 4);

                ice::pod::Array<ice::ResourceFlags> flags{ alloc };
                ice::pod::array::reserve(flags, 4);

                if (has_paths && has_flags)
                {
                    ice::meta_read_utf8_array(metadata, "resource.extra_files.paths"_sid, paths);
                    ice::meta_read_flags_array(metadata, "resource.extra_files.flags"_sid, flags);

                    ice::HeapString<char8_t> utf8_file_path{ alloc };

                    // Lets take a bet that we can use at least 20 more characters before growing!
                    ice::u32 const expected_extra_path_size = ice::string::size(meta_file) + 16;
                    ice::string::reserve(utf8_file_path, expected_extra_path_size);

                    // Create a wide string to try and check for extra files existing.
                    ice::HeapString<wchar_t> full_path{ alloc };
                    ice::string::reserve(full_path, ice::string::capacity(utf8_file_path));

                    full_path = ice::path::directory(meta_file);
                    ice::string::push_back(full_path, L'/');

                    // Remember the size so we can quickly resize.
                    ice::u32 const base_dir_size = ice::string::size(full_path);

                    for (ice::u32 idx = 0; idx < ice::pod::array::size(paths); ++idx)
                    {
                        ICE_ASSERT(
                            flags[idx] != ice::ResourceFlags::None,
                            "Extra files need to be declared with specific flags!"
                        );

                        ice::string::resize(full_path, base_dir_size);
                        utf8_to_wide_append(paths[idx], full_path);

                        ice::win32::SHHandle extra_handle = win32_open_file(full_path, FILE_ATTRIBUTE_NORMAL);
                        if (extra_handle)
                        {
                            // We know the file can be opened so we save it as a extra resource.
                            wide_to_utf8(full_path, utf8_file_path);
                            ice::path::normalize(utf8_file_path);

                            ice::Resource_LooseFilesWin32::ExtraResource* const extra_resource = alloc.make<ice::Resource_LooseFilesWin32::ExtraResource>(
                                *main_resource,
                                ice::move(utf8_file_path),
                                flags[idx]
                            );

                            ice::pod::array::push_back(out_resources, extra_resource);
                        }
                    }
                }
            }
        }
    }


} // namespace ice

#endif // #if ISP_WINDOWS
