#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    static constexpr ice::StringID scheme_file = "file"_sid;
    static constexpr ice::StringID scheme_dynlib = "dynlib"_sid;
    static constexpr ice::StringID scheme_urn = "urn"_sid;

    static constexpr ice::StringID scheme_invalid = "<invalid>"_sid;

    struct URI_v2
    {
        constexpr URI_v2() noexcept;
        constexpr URI_v2(ice::URI_v2 const& other) noexcept = default;

        constexpr explicit URI_v2(char8_t const* uri_raw) noexcept;
        constexpr explicit URI_v2(ice::Utf8String uri) noexcept;

        constexpr URI_v2(ice::StringID_Arg scheme, ice::Utf8String path) noexcept;
        constexpr URI_v2(ice::StringID_Arg scheme, ice::Utf8String path, ice::StringID_Arg fragment) noexcept;

        ice::StringID_Hash scheme;
        ice::StringID fragment;
        ice::Utf8String path;
    };


    namespace detail
    {

        constexpr auto scheme_from_uri(ice::Utf8String uri) noexcept -> ice::StringID_Hash
        {
            ice::u32 const loc = ice::string::find_first_of(uri, u8':');

            if (loc == ice::string_npos)
            {
                return ice::stringid_hash(ice::scheme_invalid);
            }
            else
            {
                return ice::stringid_hash(ice::stringid(ice::string::substr(uri, 0, loc)));
            }
        }

        constexpr auto fragment_from_uri(ice::Utf8String uri) noexcept -> ice::StringID
        {
            ice::u32 const loc = ice::string::find_last_of(uri, u8'#');

            if (loc == ice::string_npos)
            {
                return ice::stringid_invalid;
            }
            else
            {
                return ice::stringid(ice::string::substr(uri, loc + 1));
            }
        }

        constexpr auto path_from_uri(ice::Utf8String uri) noexcept
        {
            ice::u32 scheme_loc = ice::string::find_first_of(uri, u8':');
            ice::u32 fragment_loc = ice::string::find_last_of(uri, u8'#');

            if (scheme_loc == ice::string_npos)
            {
                scheme_loc = 0;
            }
            else
            {
                scheme_loc += 1;
                scheme_loc += ice::u32{ uri[scheme_loc] == '/' };
                scheme_loc += ice::u32{ uri[scheme_loc] == '/' };
            }

            if (fragment_loc != ice::string_npos)
            {
                fragment_loc -= scheme_loc;
            }

            return ice::string::substr(uri, scheme_loc, fragment_loc);
        }

        //constexpr auto path_from_uri(ice::String uri) noexcept
        //{
        //    ice::u32 scheme_loc = ice::string::find_first_of(uri, ':');
        //    ice::u32 query_loc = ice::string::find_first_of(uri, '?');
        //    ice::u32 fragment_loc = ice::string::find_last_of(uri, '#');

        //    if (scheme_loc == ice::string_npos)
        //    {
        //        scheme_loc = 0;
        //    }
        //    else
        //    {
        //        // We move past the following sequence: `://`
        //        scheme_loc += 3;
        //    }

        //    ice::u32 length = ice::string_npos;
        //    if (query_loc != ice::string_npos)
        //    {
        //        length = query_loc - scheme_loc;
        //    }
        //    else if (fragment_loc != ice::string_npos)
        //    {
        //        length = fragment_loc - scheme_loc;
        //    }

        //    return ice::string::substr(uri, scheme_loc, length);
        //}

        //constexpr auto query_from_uri(ice::String uri) noexcept
        //{
        //    u32 query_loc = ice::string::find_first_of(uri, '?');
        //    u32 fragment_loc = ice::string::find_last_of(uri, '#');

        //    if (query_loc == ice::string_npos)
        //    {
        //        return ice::String{ };
        //    }

        //    if (fragment_loc != ice::string_npos)
        //    {
        //        fragment_loc -= query_loc;
        //    }

        //    return ice::string::substr(uri, query_loc, fragment_loc);
        //}

    } // namespace detail


    constexpr URI_v2::URI_v2() noexcept
        : scheme{ ice::stringid_hash(ice::scheme_invalid) }
        , fragment{ ice::stringid_invalid }
        , path{ u8"" }
    { }

    constexpr URI_v2::URI_v2(char8_t const* uri_raw) noexcept
        : URI_v2{ ice::Utf8String{ uri_raw } }
    { }

    constexpr URI_v2::URI_v2(ice::Utf8String uri) noexcept
        : scheme{ detail::scheme_from_uri(uri) }
        , fragment{ detail::fragment_from_uri(uri) }
        , path{ detail::path_from_uri(uri) }
    { }

    constexpr URI_v2::URI_v2(ice::StringID_Arg scheme, ice::Utf8String path) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ ice::stringid_hash(ice::stringid_invalid) }
        , path{ path }
    { }

    constexpr URI_v2::URI_v2(ice::StringID_Arg scheme, ice::Utf8String path, ice::StringID_Arg fragment) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ fragment }
        , path{ path }
    { }

    constexpr auto operator""_uri(char8_t const* raw_uri, std::size_t length) noexcept -> ice::URI_v2
    {
        return URI_v2{ ice::Utf8String{ raw_uri, length } };
    }

    constexpr static ice::URI_v2 uri_invalid = u8""_uri;

} // namespace ice
