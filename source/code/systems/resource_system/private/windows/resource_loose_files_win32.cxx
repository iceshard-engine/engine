#include "resorce_file_win32.hxx"
#include <ice/assert.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/collections.hxx>

#include "../path_utils.hxx"

#if ISP_WINDOWS

namespace ice
{

    bool utf8_to_wide_append(ice::Utf8String path, ice::HeapString<wchar_t>& out_str) noexcept
    {
        ice::i32 const required_size = MultiByteToWideChar(
            CP_UTF8,
            0,
            (const char*)ice::string::data(path),
            ice::as_i32<NC_SIGN>(ice::string::size(path)),
            NULL,
            0
        );

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            ice::i32 const chars_written = MultiByteToWideChar(
                CP_UTF8,
                0,
                (const char*)ice::string::data(path),
                ice::as_i32<NC_SIGN>(ice::string::size(path)),
                ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size
            );

            ICE_ASSERT(
                chars_written == required_size,
                "The predicted string size seems to be off! [{} != {}]",
                chars_written,
                required_size
            );
        }

        return required_size != 0;
    }

    auto utf8_to_wide(ice::Allocator& alloc, ice::Utf8String path) noexcept -> ice::HeapString<wchar_t>
    {
        ice::HeapString<wchar_t> result{ alloc };
        utf8_to_wide_append(path, result);
        return result;
    }

    auto wide_to_utf8(ice::WString path, ice::HeapString<char8_t>& out_str) noexcept
    {
        ice::i32 const required_size = WideCharToMultiByte(
            CP_UTF8,
            0,
            ice::string::data(path),
            ice::as_i32<NC_SIGN>(ice::string::size(path)),
            NULL,
            0,
            NULL,
            NULL
        );

        if (required_size != 0)
        {
            ice::u32 const current_size = ice::string::size(out_str);
            ice::u32 const total_size = static_cast<ice::u32>(required_size) + ice::string::size(out_str);
            ice::string::resize(out_str, total_size);

            ice::i32 const chars_written = WideCharToMultiByte(
                CP_UTF8,
                0,
                ice::string::data(path),
                ice::as_i32<NC_SIGN>(ice::string::size(path)),
                (char*)ice::string::begin(out_str) + current_size,
                ice::string::size(out_str) - current_size,
                NULL,
                NULL
            );

            ICE_ASSERT(
                chars_written == required_size,
                "The predicted string size seems to be off! [{} != {}]",
                chars_written,
                required_size
            );
        }

        return required_size != 0;
    }

    auto wide_to_utf8_size(ice::WString path) noexcept -> ice::u32
    {
        ice::i32 const required_size = WideCharToMultiByte(
            CP_UTF8,
            0,
            ice::string::data(path),
            ice::as_i32<NC_SIGN>(ice::string::size(path)),
            NULL,
            0,
            NULL,
            NULL
        );
        return static_cast<ice::u32>(required_size);
    }


    auto win32_open_file(ice::WString path) noexcept -> ice::win32::SHHandle
    {
        ice::win32::SHHandle handle = CreateFile(
            ice::string::data(path),
            GENERIC_READ,
            0, // FILE_SHARE_*
            NULL, // SECURITY ATTRIBS
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, // At some point we might add: FILE_FLAG_OVERLAPPED, FILE_FLAG_RANDOM_ACCESS
            NULL
        );
        return handle;
    }

    bool win32_load_file(ice::win32::SHHandle const& handle, ice::Buffer& out_buffer) noexcept
    {
        ice::memory::StackAllocator_4096 kib4_alloc;
        ice::Memory memory_chunk = {
            .location = kib4_alloc.allocate(kib4_alloc.Constant_BufferSize),
            .size = kib4_alloc.Constant_BufferSize,
            .alignment = 1 // Characters are not aligned to any particular number
        };

        BOOL result = FALSE;
        DWORD characters_read = 0;
        do
        {
            result = ReadFile(
                handle.native(),
                memory_chunk.location,
                memory_chunk.size,
                &characters_read,
                nullptr
            );

            if (characters_read > 0)
            {
                ice::buffer::append(
                    out_buffer,
                    memory_chunk.location,
                    characters_read,
                    memory_chunk.alignment
                );
            }

        } while (characters_read == 0 && result != FALSE);

        kib4_alloc.deallocate(memory_chunk.location);
        return result != FALSE;
    }

    auto create_resource_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_v2*
    {
        ice::win32::SHHandle meta_handle = win32_open_file(meta_file);
        ice::win32::SHHandle data_handle = win32_open_file(data_file);

        ice::Resource_v2* result = nullptr;
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
                ice::pod::array::push_back(paths, utf8_data_file_path);

                ice::pod::Array<ice::i32> flags{ alloc };
                ice::pod::array::push_back(flags, 0);

#if 0
                if (has_paths && has_flags)
                {
                    ice::meta_read_utf8_array(mutable_meta, "resource.extra_files.paths"_sid, paths);
                    ice::meta_read_int32_array(mutable_meta, "resource.extra_files.flags"_sid, flags);

                    ice::WString const base_dir = ice::path::directory(meta_file);

                    ice::Vector<ice::win32::SHHandle> extra_handles{ alloc };
                    extra_handles.reserve(ice::pod::array::size(paths) + 1);
                    extra_handles.push_back(ice::move(data_handle));

                    ice::HeapString<wchar_t> full_path{ alloc, base_dir };
                    ice::u32 const base_dir_size = ice::string::size(base_dir);

                    for (ice::Utf8String const& file_path : paths)
                    {
                        ice::string::resize(full_path, base_dir_size);
                        utf8_to_wide_append(file_path, full_path);

                        ice::win32::SHHandle extra_handle = win32_open_file(full_path);
                        //ICE_ASSERT(
                        //    extra_handle == true,
                        //    "Extra file '{}' couldn't be opened.",
                        //    file_path
                        //);

                        if (!extra_handle)
                        {
                            extra_handle = HANDLE{ };
                        }

                        extra_handles.push_back(ice::move(extra_handle));
                    }


                }
#endif

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

    Resource_LooseFilesWin32::Resource_LooseFilesWin32(
        ice::MutableMetadata metadata,
        ice::pod::Array<ice::Utf8String> data_files,
        ice::pod::Array<ice::i32> data_files_flags,
        ice::HeapString<char8_t> origin_path,
        ice::Utf8String origin_name
    ) noexcept
        : ice::Resource_v2{ }
        , _metadata{ ice::move(metadata) }
        , _data_files{ ice::move(data_files) }
        , _data_files_flags{ ice::move(data_files_flags) }
        , _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
    {
    }

    Resource_LooseFilesWin32::~Resource_LooseFilesWin32() noexcept
    {
    }

} // namespace ice

#endif // #if ISP_WINDOWS
