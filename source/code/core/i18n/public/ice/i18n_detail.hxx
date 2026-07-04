#pragma once
#include <ice/string.hxx>
#include <fmt/format.h>

namespace ice::i18n
{

    struct I18NFlags
    {
        constexpr explicit I18NFlags(ice::String data) noexcept
            : _data{ data }
        {
            if (_data.not_empty() && _data.back() == '|')
            {
                _data = _data.substr(0, _data.size() - 1);
            }
        }

        constexpr bool operator==(ice::String rhs) const noexcept { return _data == rhs; }

        constexpr auto language(fmt::format_args const& args) const noexcept -> ice::String;

        ice::String _data;
    };

    namespace detail
    {

        template<typename ArgType>
        struct FormatArgVisitor
        {
            ArgType& _reference;

            constexpr bool operator()(ArgType arg) const noexcept
            {
                _reference = arg;
                return true;
            }

            constexpr bool operator()(auto const& unused) const noexcept
            {
                return false;
            }
        };

    } // namespace detail

    constexpr auto I18NFlags::language(fmt::format_args const& args) const noexcept -> ice::String
    {
        ice::nindex const lang_start = _data.find_first_of('@');
        ice::nindex const lang_end = _data.find_first_of(", |", lang_start);
        if (lang_start == ice::nindex_none)
        {
            return { };
        }

        ice::String const lang = _data.substr(lang_start + 1, lang_end - lang_start - 1);
        if (lang[0] == '{')
        {
            ice::nindex const lang_arg_end = lang.find_first_of('}');
            if (lang_arg_end != std::string_view::npos && lang_arg_end > 1)
            {
                ice::i32 const id = atol(lang.data());
                // if (ice::from_chars(lang, id) == S_Ok)
                {
                    ice::String result{};
                    {
                        // Find the format arg result
                        ice::i18n::detail::FormatArgVisitor<char const*> lang_arg_visitor{ result._data };
                        args.get(id).visit(lang_arg_visitor);
                    }
                    result._count = ice::string::detail::strptr_size(result._data);
                    return result;
                }
            }
        }

        return lang;
    }

    constexpr void parse(
        ice::String in_str,
        ice::u32& out_hash,
        ice::u8& out_path,
        ice::u8& out_key,
        ice::u8& out_flags,
        ice::u8& out_fallback
    ) noexcept
    {
        ice::nindex const path_sep = in_str.find_first_of('/');
        ICE_ASSERT_CORE(path_sep != ice::nindex_none);
        out_path = (path_sep + 1).u8();

        ice::String str = in_str.substr(path_sep + 1);
        ice::nindex const key_sep = str.find_first_of(" |");
        ice::String const key = str.substr(0, key_sep);
        out_key = key.size().u8();

        // Create a hash from the reference value. `a.path/a.key`
        out_hash = ice::hash32(in_str.substr(key_sep + 1));

        // Find additional params or the fallback string
        if (key_sep.is_valid())
        {
            str = str.substr(key_sep);
            if (str[0] == ' ')
            {
                ice::nindex const flags_sep = str.find_first_of('|');
                ICE_ASSERT_CORE(flags_sep.value_or(2) >= 2); // We need at least one more character for flags to be valid!

                if (flags_sep.is_valid())
                {
                    out_flags = flags_sep.u8() - 1u;
                    out_fallback = (str.size() - flags_sep).u8() - 1u;
                }
                else
                {
                    out_flags = str.size().u8() - 1u;
                }
            }
            else
            {
                out_flags = 0;
                out_fallback = str.size().u8() - 1u;
            }
        }
    }

} // namespace ice::i18n
