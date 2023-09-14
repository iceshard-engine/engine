/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    static constexpr ice::StringID Scheme_URN = "urn"_sid;
    static constexpr ice::StringID Scheme_File = "file"_sid;
    static constexpr ice::StringID Scheme_Dynlib = "dynlib"_sid;

    static constexpr ice::StringID Scheme_Invalid = "<invalid>"_sid;

    struct URI
    {
        constexpr URI() noexcept;
        constexpr URI(ice::URI const& other) noexcept = default;

        constexpr explicit URI(char const* uri_raw) noexcept;
        constexpr explicit URI(ice::String uri) noexcept;

        constexpr URI(ice::StringID_Arg scheme, ice::String path) noexcept;
        constexpr URI(ice::StringID_Arg scheme, ice::String path, ice::StringID_Arg fragment) noexcept;

        ice::StringID_Hash scheme;
        ice::StringID fragment;
        ice::String path;
    };


    namespace detail
    {

        constexpr auto scheme_from_uri(ice::String uri) noexcept -> ice::StringID_Hash
        {
            ice::u32 const loc = ice::string::find_first_of(uri, ':');

            if (loc == ice::String_NPos)
            {
                return ice::stringid_hash(ice::Scheme_Invalid);
            }
            else
            {
                return ice::stringid_hash(ice::stringid(ice::string::substr(uri, 0, loc)));
            }
        }

        constexpr auto fragment_from_uri(ice::String uri) noexcept -> ice::StringID
        {
            ice::u32 const loc = ice::string::find_last_of(uri, '#');

            if (loc == ice::String_NPos)
            {
                return ice::StringID_Invalid;
            }
            else
            {
                return ice::stringid(ice::string::substr(uri, loc + 1));
            }
        }

        constexpr auto path_from_uri(ice::String uri) noexcept
        {
            ice::u32 const size = ice::string::size(uri);
            ice::u32 scheme_loc = ice::string::find_first_of(uri, ':');
            ice::u32 fragment_loc = ice::string::find_last_of(uri, '#');

            if (scheme_loc == ice::String_NPos)
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

            if (fragment_loc != ice::String_NPos)
            {
                fragment_loc -= scheme_loc;
            }

            return ice::string::substr(uri, scheme_loc, fragment_loc);
        }

        static_assert(path_from_uri("file:path/name.file") == "path/name.file");
        static_assert(path_from_uri("file:/path/name.file") == "/path/name.file");
        static_assert(path_from_uri("file://path/name.file") == "path/name.file");
        static_assert(path_from_uri("file:///path/name.file") == "/path/name.file");
        static_assert(scheme_from_uri("file:path/name.file") == ice::Scheme_File);
        static_assert(scheme_from_uri("file:/path/name.file") == ice::Scheme_File);
        static_assert(scheme_from_uri("file://path/name.file") == ice::Scheme_File);
        static_assert(scheme_from_uri("file:///path/name.file") == ice::Scheme_File);

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
        : scheme{ ice::stringid_hash(ice::Scheme_Invalid) }
        , fragment{ ice::StringID_Invalid }
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
        , fragment{ ice::stringid_hash(ice::StringID_Invalid) }
        , path{ path }
    { }

    constexpr URI::URI(ice::StringID_Arg scheme, ice::String path, ice::StringID_Arg fragment) noexcept
        : scheme{ ice::stringid_hash(scheme) }
        , fragment{ fragment }
        , path{ path }
    { }

    constexpr auto operator""_uri(char const* raw_uri, std::size_t length) noexcept -> ice::URI
    {
        return URI{ ice::String{ raw_uri, ice::ucount(length) } };
    }

    constexpr static ice::URI URI_Invalid = ""_uri;

} // namespace ice
