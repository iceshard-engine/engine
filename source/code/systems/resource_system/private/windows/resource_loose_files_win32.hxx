#pragma once
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/os/windows.hxx>
#include <ice/collections.hxx>
#include <ice/heap_string.hxx>
#include <ice/uri.hxx>

#include "resource_common_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class Resource_LooseFilesWin32 final : public ice::Resource_Win32
    {
    public:
        Resource_LooseFilesWin32(
            ice::MutableMetadata metadata,
            ice::pod::Array<ice::Utf8String> data_files,
            ice::pod::Array<ice::i32> data_files_flags,
            ice::HeapString<char8_t> origin_path,
            ice::Utf8String origin_name
        ) noexcept;

        ~Resource_LooseFilesWin32() noexcept override;

        auto uri() const noexcept -> ice::URI const& override;
        auto name() const noexcept -> ice::Utf8String override;
        auto origin() const noexcept -> ice::Utf8String override;

        auto metadata() const noexcept -> ice::Metadata const& override;

        auto load_data_for_flags(
            ice::Allocator& alloc,
            ice::u32 flags,
            ice::TaskScheduler_v2& scheduler
        ) const noexcept -> ice::Task<ice::Memory> override;

    private:
        ice::MutableMetadata _mutable_metadata;
        ice::Metadata _metadata;

        ice::pod::Array<ice::Utf8String> _data_files;
        ice::pod::Array<ice::i32> _data_files_flags;
        ice::HeapString<char8_t> _origin_path;
        ice::Utf8String _origin_name;

        ice::URI _uri;
    };

    auto create_resource_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_Win32*;

} // namespace ice

#endif // #if ISP_WINDOWS
