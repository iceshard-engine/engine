#pragma once
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/collections.hxx>
#include <ice/heap_string.hxx>
#include <ice/os/windows.hxx>

#if ISP_WINDOWS

namespace ice
{

    bool utf8_to_wide_append(ice::Utf8String path, ice::HeapString<wchar_t>& out_str) noexcept;
    auto utf8_to_wide(ice::Allocator& alloc, ice::Utf8String path) noexcept -> ice::HeapString<wchar_t>;

    auto win32_open_file(ice::WString path) noexcept -> ice::win32::SHHandle;
    bool win32_load_file(ice::win32::SHHandle const& handle, ice::Buffer& out_buffer) noexcept;

    auto create_resource_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_v2*;

    class Resource_FileWin32 final : public ice::Resource_v2
    {
    public:
        Resource_FileWin32(ice::String path) noexcept;

        virtual ~Resource_FileWin32() noexcept override;

        auto uri() const noexcept -> ice::URI_v2 const&
        {
            return { };
        }

        auto name() const noexcept -> ice::Utf8String
        {
            return { };
        }

        auto origin() const noexcept -> ice::Utf8String
        {
            return { };
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
        Resource_LooseFilesWin32(
            ice::MutableMetadata metadata,
            ice::pod::Array<ice::Utf8String> data_files,
            ice::pod::Array<ice::i32> data_files_flags,
            ice::HeapString<char8_t> origin_path,
            ice::Utf8String origin_name
        ) noexcept;

        virtual ~Resource_LooseFilesWin32() noexcept override;

        auto uri() const noexcept -> ice::URI_v2 const&
        {
            static auto foo = ice::URI_v2{ ice::scheme_file, _origin_name };
            return foo;
        }

        auto name() const noexcept -> ice::Utf8String
        {
            return _origin_name;
        }

        auto origin() const noexcept -> ice::Utf8String
        {
            return _origin_path;
        }

        auto metadata() const noexcept -> ice::Metadata const&
        {
            static ice::Metadata meta = _metadata;
            return meta;
        }

    private:
        ice::MutableMetadata _metadata;
        ice::pod::Array<ice::Utf8String> _data_files;
        ice::pod::Array<ice::i32> _data_files_flags;
        ice::HeapString<char8_t> _origin_path;
        ice::Utf8String _origin_name;
    };

} // namespace ice

#endif // #if ISP_WINDOWS
