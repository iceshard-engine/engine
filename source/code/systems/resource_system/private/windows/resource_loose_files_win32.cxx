#include "resource_loose_files_win32.hxx"
#include <ice/assert.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/collections.hxx>

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
        ice::pod::Array<ice::Utf8String> data_files,
        ice::pod::Array<ice::i32> data_files_flags,
        ice::HeapString<char8_t> origin_path,
        ice::Utf8String origin_name
    ) noexcept
        : ice::Resource_Win32{ }
        , _mutable_metadata{ ice::move(metadata) }
        , _metadata{ _mutable_metadata }
        , _data_files{ ice::move(data_files) }
        , _data_files_flags{ ice::move(data_files_flags) }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri{ ice::scheme_file, _origin_name }
    {
    }

    Resource_LooseFilesWin32::~Resource_LooseFilesWin32() noexcept
    {
    }

    auto Resource_LooseFilesWin32::uri() const noexcept -> ice::URI_v2 const&
    {
        return _uri;
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
        ice::Utf8String file_location = ice::pod::array::front(_data_files);
        ICE_ASSERT(flags == 0, "Only default flags value is supported for now!");

        //if (flags != 0)
        //{
        //    for (ice::u32 idx = 1; idx < ice::pod::array::size(_data_files); ++idx)
        //    {
        //        if ((_data_files_flags[idx] & flags) == flags)
        //        {
        //            file_location = _data_files[idx];
        //        }
        //    }
        //}

        ice::HeapString<wchar_t> file_location_wide{ alloc };
        ice::utf8_to_wide_append(_origin_path, file_location_wide);

        //ice::WString directory = ice::path::directory(file_location_wide);
        //ice::string::resize(file_location_wide, ice::string::size(directory) + 1);
        //ice::utf8_to_wide_append(file_location, file_location_wide);

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

    auto create_resource_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_Win32*
    {
        ice::win32::SHHandle meta_handle = win32_open_file(meta_file, FILE_ATTRIBUTE_NORMAL);
        ice::win32::SHHandle data_handle = win32_open_file(data_file, FILE_ATTRIBUTE_NORMAL);

        ice::Resource_Win32* result = nullptr;
        if (meta_handle && data_handle)
        {
            ice::Buffer meta_file_data{ alloc };

            if (win32_load_file(meta_handle, meta_file_data)
                && ice::buffer::size(meta_file_data) > ice::string::size(Constant_FileHeader_MetadataFile))
            {
                ice::String const file_header{
                    reinterpret_cast<char const*>(ice::buffer::data(meta_file_data)),
                    ice::string::size(Constant_FileHeader_MetadataFile)
                };

                if (Constant_FileHeader_MetadataFile == file_header)
                {
                    return nullptr;
                }

                ice::HeapString<char8_t> utf8_data_file_path{ alloc };
                wide_to_utf8(data_file, utf8_data_file_path);

                ice::Utf8String utf8_origin_name = ice::string::substr(utf8_data_file_path, wide_to_utf8_size(base_path));

                // We have a loose resource files which contain metadata associated data.
                // We need now to read the metadata and check if there are more file associated and if all are available.
                ice::MutableMetadata mutable_meta{ alloc };
                ice::meta_deserialize(meta_file_data, mutable_meta);

                bool const has_paths = ice::meta_has_entry(mutable_meta, "resource.extra_files.paths"_sid);
                bool const has_flags = ice::meta_has_entry(mutable_meta, "resource.extra_files.flags"_sid);
                ICE_ASSERT(
                    (has_paths ^ has_flags) == 0,
                    "If a resource defines extra files, it's only allowed to do so by providing 'unique flags' and co-related 'relative paths'."
                );

                ice::pod::Array<ice::Utf8String> paths{ alloc };
                ice::pod::array::reserve(paths, 4);
                ice::pod::array::push_back(paths, utf8_data_file_path);

                ice::pod::Array<ice::i32> flags{ alloc };
                ice::pod::array::reserve(flags, 4);
                ice::pod::array::push_back(flags, 0);

                if (has_paths && has_flags)
                {
                    ice::meta_read_utf8_array(mutable_meta, "resource.extra_files.paths"_sid, paths);
                    ice::meta_read_int32_array(mutable_meta, "resource.extra_files.flags"_sid, flags);

                    ice::WString const base_dir = ice::path::directory(meta_file);

                    ice::Vector<ice::win32::SHHandle> extra_handles{ alloc };
                    extra_handles.reserve(ice::pod::array::size(paths) + 1);
                    extra_handles.push_back(ice::move(data_handle));

                    ice::HeapString<wchar_t> full_path{ alloc, base_dir };
                    ice::string::push_back(full_path, L'/');
                    ice::u32 const base_dir_size = ice::string::size(base_dir) + 1;

                    for (ice::Utf8String& file_path : ice::pod::array::span(paths, 1))
                    {
                        ice::string::resize(full_path, base_dir_size);
                        utf8_to_wide_append(file_path, full_path);

                        ice::win32::SHHandle extra_handle = win32_open_file(full_path, FILE_ATTRIBUTE_NORMAL);
                        //ICE_ASSERT(
                        //    extra_handle == true,
                        //    "Extra file '{}' couldn't be opened.",
                        //    file_path
                        //);

                        if (!extra_handle)
                        {
                            file_path = {};
                            extra_handle = HANDLE{ };
                        }

                        extra_handles.push_back(ice::move(extra_handle));
                    }
                }

                result = alloc.make<ice::Resource_LooseFilesWin32>(
                    ice::move(mutable_meta),
                    ice::move(paths),
                    ice::move(flags),
                    ice::move(utf8_data_file_path),
                    utf8_origin_name
                    );
            }
        }

        return result;
    }

} // namespace ice

#endif // #if ISP_WINDOWS
