#pragma once
#include <ice/string.hxx>
#include <ice/heap_string.hxx>

namespace ice::path
{

    static constexpr ice::String separators_extension = ".";

    static constexpr ice::String separators_directory = "/\\";

    [[nodiscard]]
    bool is_absolute(ice::String path) noexcept;

    [[nodiscard]]
    auto extension(ice::String path) noexcept -> ice::String;

    [[nodiscard]]
    auto filename(ice::String path) noexcept -> ice::String;

    [[nodiscard]]
    auto directory(ice::String path) noexcept -> ice::String;

    auto normalize(ice::HeapString<>& path) noexcept -> ice::String;

    auto join(ice::HeapString<>& left, ice::String right) noexcept -> ice::String;

    auto replace_filename(ice::HeapString<>& str, ice::String filename) noexcept -> ice::String;

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
