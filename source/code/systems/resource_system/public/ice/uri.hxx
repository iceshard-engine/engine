#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    static constexpr ice::StringID scheme_urn = "urn"_sid;
    static constexpr ice::StringID scheme_file = "file"_sid;
    static constexpr ice::StringID scheme_dynlib = "dynlib"_sid;

    static constexpr ice::StringID scheme_invalid = "<invalid>"_sid;

    struct URI
    {
        constexpr URI() noexcept;
        constexpr URI(ice::URI const& other) noexcept = default;

        constexpr explicit URI(char8_t const* uri_raw) noexcept;
        constexpr explicit URI(ice::Utf8String uri) noexcept;

        constexpr URI(ice::StringID_Arg scheme, ice::Utf8String path) noexcept;
        constexpr URI(ice::StringID_Arg scheme, ice::Utf8String path, ice::StringID_Arg fragment) noexcept;

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
            ice::u32 const size = ice::string::size(uri);
            ice::u32 scheme_loc = ice::string::find_first_of(uri, u8':');
            ice::u32 fragment_loc = ice::string::find_last_of(uri, u8'#');

            if (scheme_loc == ice::string_npos)
            {
                scheme_loc = 0;
            }
            else
            {
                scheme_loc += 1;

                if (size > (scheme_loc + 1))
                {
                    // The following series operations allows us to properly handle the optional '//' prefix in URI paths.
                    //  Some examples: (1: 'file:/asd' -> '/asd'), (2: 'file:///asd' -> '/asd'), (3: 'file://asd' -> 'asd')
                    //
                    // NOTE: last one does not have the preceding '/' because it's seen as the optional double '/' for URI's.

                    // (check the current and next character for slashes)
                    scheme_loc += ice::u32{ uri[scheme_loc] == '/' } *ice::u32{ uri[scheme_loc + 1] == '/' } *2;
                }
            }

            if (fragment_loc != ice::string_npos)
            {
                fragment_loc -= scheme_loc;
            }

            return ice::string::substr(uri, scheme_loc, fragment_loc);
        }

        static_assert(path_from_uri(u8"file:path/name.file") == u8"path/name.file"_str);
        static_assert(path_from_uri(u8"file:/path/name.file") == u8"/path/name.file"_str);
        static_assert(path_from_uri(u8"file://path/name.file") == u8"path/name.file"_str);
        static_assert(path_from_uri(u8"file:///path/name.file") == u8"/path/name.file"_str);

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


    constexpr URI::URI() noexcept
        : scheme{ ice::stringid_hash(ice::scheme_invalid) }
        , fragment{ ice::stringid_invalid }
        , path{ u8"" }
    { }

    constexpr URI::URI(char8_t const* uri_raw) noexcept
        : URI{ ice::Utf8String{ uri_raw } }
    { }

    constexpr URI::URI(ice::Utf8String uri) noexcept
        : scheme{ detail::scheme_from_uri(uri) }
        , fragment{ detail::fragment_from_uri(uri) }
        , path{ detail::path_from_uri(uri) }
    { }

    constexpr URI::URI(ice::StringID_Arg scheme, ice::Utf8String path) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ ice::stringid_hash(ice::stringid_invalid) }
        , path{ path }
    { }

    constexpr URI::URI(ice::StringID_Arg scheme, ice::Utf8String path, ice::StringID_Arg fragment) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ fragment }
        , path{ path }
    { }

    constexpr auto operator""_uri(char8_t const* raw_uri, std::size_t length) noexcept -> ice::URI
    {
        return URI{ ice::Utf8String{ raw_uri, length } };
    }

    constexpr static ice::URI uri_invalid = u8""_uri;

} // namespace ice
