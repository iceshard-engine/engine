#include "resource_dll_win32.hxx"
#include <ice/resource_meta.hxx>

#include "resource_utils_win32.hxx"

namespace ice
{

    Resource_DllsWin32::Resource_DllsWin32(
        ice::HeapString<char8_t> origin_path,
        ice::Utf8String origin_name
    ) noexcept
        : _origin_path{ ice::move(origin_path) }
        , _origin_name{ origin_name }
        , _uri{ ice::scheme_dynlib, _origin_name }
    {
    }

    auto Resource_DllsWin32::uri() const noexcept -> ice::URI_v2 const&
    {
        return _uri;
    }

    auto Resource_DllsWin32::name() const noexcept -> ice::Utf8String
    {
        return _origin_name;
    }

    auto Resource_DllsWin32::origin() const noexcept -> ice::Utf8String
    {
        return _origin_path;
    }

    auto Resource_DllsWin32::metadata() const noexcept -> ice::Metadata const&
    {
        static ice::Metadata empty_meta;
        return empty_meta;
    }


    auto create_resource_from_dll_path(
        ice::Allocator& alloc,
        ice::WString dll_path
    ) noexcept -> ice::Resource_v2*
    {
        ice::Resource_v2* result = nullptr;

        DWORD const file_attribs = GetFileAttributesW(ice::string::data(dll_path));
        if (file_attribs != INVALID_FILE_ATTRIBUTES)
        {
            ice::HeapString<char8_t> utf8_data_file_path{ alloc };
            wide_to_utf8(dll_path, utf8_data_file_path);

            ice::Utf8String utf8_origin_name = ice::string::substr(utf8_data_file_path, ice::string::find_last_of(dll_path, L'/') + 1);

            result = alloc.make<ice::Resource_DllsWin32>(
                ice::move(utf8_data_file_path),
                utf8_origin_name
            );
        }

        return result;
    }

} // namespace ice
