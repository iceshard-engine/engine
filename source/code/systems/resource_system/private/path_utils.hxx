#pragma once
#include <ice/string.hxx>
#include <ice/heap_string.hxx>

namespace ice::path
{

    static constexpr ice::String separators_extension = ".";

    static constexpr ice::Utf8String  separators_directory = u8"/\\";

    [[nodiscard]]
    bool is_absolute(ice::Utf8String path) noexcept;

    [[nodiscard]]
    auto extension(ice::String path) noexcept -> ice::String;

    [[nodiscard]]
    auto filename(ice::Utf8String path) noexcept -> ice::Utf8String;

    [[nodiscard]]
    auto directory(ice::Utf8String path) noexcept -> ice::Utf8String;

    auto normalize(ice::HeapString<char8_t>& path) noexcept -> ice::Utf8String;

    auto join(ice::HeapString<char8_t>& left, ice::Utf8String right) noexcept -> ice::Utf8String;

    auto replace_filename(ice::HeapString<char8_t>& str, ice::Utf8String filename) noexcept -> ice::Utf8String;

    auto replace_extension(ice::HeapString<>& str, ice::String extension) noexcept -> ice::String;

#if ISP_WINDOWS

    static constexpr ice::WString Constant_DirectorySeparators = L"/\\";

    [[nodiscard]]
    bool is_absolute(ice::WString path) noexcept;

    [[nodiscard]]
    auto directory(ice::WString path) noexcept -> ice::WString;

    auto join(ice::HeapString<wchar_t>& left, ice::WString right) noexcept -> ice::WString;

#endif

} // namespace ice::path
