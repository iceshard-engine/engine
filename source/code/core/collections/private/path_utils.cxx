#include <core/path_utils.hxx>
#include <core/memory.hxx>

namespace core::path
{

    bool is_absolute(core::StringView path) noexcept
    {
        if constexpr(core::build::is_windows)
        {
            if (path.size() >= 3)
            {
                return path[1] == ':' && (path[2] == '\\' || path[2] == '/');
            }
            return false;
        }
        else
        {
            return false;
        }
    }

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

    auto join(core::String<>& left, core::StringView right) noexcept -> core::StringView
    {
        // This one was taken from MS's std::filesystem implementation.
        if (is_absolute(right))
        {
            return left.operator=(right);
        }

        if (auto last_char = core::string::back(left); last_char != '/')
        {
            if (last_char == '\\')
            {
                core::string::pop_back(left);
            }
            core::string::push_back(left, '/');
        }
        core::string::push_back(left, right);
        return left;
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
