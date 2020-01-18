#include <core/path_utils.hxx>
#include <core/memory.hxx>

namespace core::path
{

    auto extension(core::StringView str) noexcept -> core::StringView
    {
        auto const separator_pos = core::string::find_last_of(str, constant::extension_separator);
        return core::string::substr(str, separator_pos == core::string::npos ? core::string::size(str) : separator_pos);
    }

    auto filename(core::StringView str) noexcept -> core::StringView
    {
        auto const separator_pos = core::string::find_last_of(str, constant::directory_separators);
        return core::string::substr(str, separator_pos == core::string::npos ? 0 : separator_pos + 1);
    }

    auto directory(core::StringView str) noexcept -> core::StringView
    {
        auto const separator_pos = core::string::find_last_of(str, constant::directory_separators);
        return core::string::substr(str, separator_pos == core::string::npos ? core::string::size(str) : 0, separator_pos);
    }

    auto replace_filename(core::String<>& str, core::StringView name) noexcept -> core::StringView
    {
        auto const separator_pos = core::string::find_last_of({ str._data, str._size }, constant::directory_separators);
        if (separator_pos != core::string::npos)
        {
            core::string::resize(str, separator_pos + 1);
        }
        else
        {
            core::string::clear(str);
        }

        if (!core::string::empty(name))
        {
            core::string::push_back(str, name);
        }

        return { str._data, str._size };
    }

} // namespace core::path
