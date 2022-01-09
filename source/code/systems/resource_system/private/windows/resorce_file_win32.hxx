#pragma once
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/os/windows.hxx>
#include <ice/heap_string.hxx>

#if ISP_WINDOWS

namespace ice
{

    bool utf8_to_wide_append(ice::Utf8String path, ice::HeapString<wchar_t>& out_str) noexcept;
    auto utf8_to_wide(ice::Allocator& alloc, ice::Utf8String path) noexcept -> ice::HeapString<wchar_t>;

    auto win32_open_file(ice::WString path) noexcept -> ice::win32::SHHandle;
    bool win32_load_file(ice::win32::SHHandle const& handle, ice::Buffer& out_buffer) noexcept;

    auto create_resource_from_loose_files(
        ice::Allocator& alloc,
        ice::HeapString<wchar_t> const& meta_file,
        ice::HeapString<wchar_t> const& data_file
    ) noexcept -> ice::Resource_v2*;

    //auto create_resource_from_resource_file(ice::Allocator& alloc, ice::HeapString<wchar_t> const& path) -> ice::Resource_v2*
    //{
    //    ice::win32::SHHandle const file_handle = win32_open_file(path);

    //    ice::Resource_v2* result = nullptr;
    //    if (file_handle)
    //    {
    //        char file_header[4];

    //        DWORD chars_read = 0;
    //        BOOL const result = ReadFile(
    //            file_handle,
    //            file_header,
    //            ice::size(file_header),
    //            &chars_read,
    //            nullptr
    //        );

    //        if (result != FALSE)
    //        {
    //            if (Constant_FileHeader_ResourceFile == file_header)
    //            {
    //                // We have a baked resource file which contains the metadata and all associated data 'files'.
    //            }
    //        }
    //    }

    //    return result;
    //}


    class Resource_FileWin32 final : public ice::Resource_v2
    {
    public:
        Resource_FileWin32(ice::String path) noexcept;

        virtual ~Resource_FileWin32() noexcept override;

        auto uri() const noexcept -> ice::URI const&
        {
            return { };
        }

        auto name() const noexcept -> ice::String
        {
            return { };
        }

        auto origin() const noexcept -> ice::String
        {
            return {};
        }

        auto metadata() const noexcept -> ice::Metadata const&
        {
            return _metadata;
        }

    private:
        ice::Metadata _metadata;
        ice::win32::SHHandle _file_handle;
    };

    class Resource_LooseFilesWin32 final : public ice::Resource_v2
    {
    public:
        Resource_LooseFilesWin32(ice::String path) noexcept;

        virtual ~Resource_LooseFilesWin32() noexcept override;

        auto uri() const noexcept -> ice::URI const&
        {
            return { };
        }

        auto name() const noexcept -> ice::String
        {
            return { };
        }

        auto origin() const noexcept -> ice::String
        {
            return {};
        }

        auto metadata() const noexcept -> ice::Metadata const&
        {
            return _metadata;
        }

    private:
        ice::MutableMetadata _metadata;
    };

} // namespace ice

#endif // #if ISP_WINDOWS
