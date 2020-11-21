#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>
#include <ice/urn.hxx>

namespace ice
{

    static constexpr ice::StringID scheme_file = "file"_sid;
    static constexpr ice::StringID scheme_directory = "dir"_sid;
    static constexpr ice::StringID scheme_pack = "pack"_sid;
    static constexpr ice::StringID scheme_dynlib = "dynlib"_sid;
    static constexpr ice::StringID scheme_resource = "res"_sid;

    static constexpr ice::StringID scheme_invalid = "<invalid>"_sid;

    struct URI
    {
        constexpr URI() noexcept;
        constexpr explicit URI(char const* uri_raw) noexcept;
        constexpr explicit URI(ice::String uri) noexcept;

        constexpr URI(ice::StringID_Arg scheme, ice::String path) noexcept;
        constexpr URI(ice::StringID_Arg scheme, ice::String path, ice::StringID_Arg fragment) noexcept;

        constexpr URI(URI const& other) noexcept;

        ice::StringID_Hash scheme;

        ice::StringID fragment;

        ice::String path;
    };


    namespace detail
    {

        constexpr auto scheme_from_uri(ice::String uri) noexcept -> ice::StringID_Hash
        {
            u32 const loc = ice::string::find_first_of(uri, ':');

            if (loc == ice::string_npos)
            {
                return ice::stringid_hash(ice::scheme_invalid);
            }
            else
            {
                return ice::stringid_hash(ice::stringid(ice::string::substr(uri, 0, loc)));
            }
        }

        constexpr auto fragment_from_uri(ice::String uri) noexcept -> ice::StringID
        {
            u32 const loc = ice::string::find_last_of(uri, '#');

            if (loc == ice::string_npos)
            {
                return ice::stringid_invalid;
            }
            else
            {
                return ice::stringid(ice::string::substr(uri, loc + 1));
            }
        }

        constexpr auto path_from_uri(ice::String uri) noexcept
        {
            u32 scheme_loc = ice::string::find_first_of(uri, ':');
            u32 fragment_loc = ice::string::find_last_of(uri, '#');

            if (scheme_loc == ice::string_npos)
            {
                scheme_loc = 0;
            }
            else
            {
                // We move past the following sequence: `://`
                scheme_loc += 3;
            }

            if (fragment_loc != ice::string_npos)
            {
                fragment_loc -= scheme_loc;
            }

            return ice::string::substr(uri, scheme_loc, fragment_loc);
        }

    } // namespace detail

    constexpr URI::URI() noexcept
        : scheme{ ice::stringid_hash(ice::scheme_invalid) }
        , fragment{ ice::stringid_invalid }
        , path{ "" }
    { }

    constexpr URI::URI(char const* uri_raw) noexcept
        : URI{ ice::String{ uri_raw } }
    { }

    constexpr URI::URI(ice::String uri) noexcept
        : scheme{ detail::scheme_from_uri(uri) }
        , fragment{ detail::fragment_from_uri(uri) }
        , path{ detail::path_from_uri(uri) }
    { }

    constexpr URI::URI(ice::StringID_Arg scheme, ice::String path) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ ice::stringid_hash(ice::stringid_invalid) }
        , path{ path }
    { }

    constexpr URI::URI(ice::StringID_Arg scheme, ice::String path, ice::StringID_Arg fragment) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ fragment }
        , path{ path }
    { }

    constexpr URI::URI(URI const& other) noexcept
        : scheme{ other.scheme }
        , fragment{ other.fragment }
        , path{ other.path }
    { }

    constexpr auto operator""_uri(char const* raw_uri, std::size_t length) noexcept -> ice::URI
    {
        return URI{ ice::String{ raw_uri, length } };
    }

    constexpr static ice::URI uri_invalid = ""_uri;

} // namespace ice
