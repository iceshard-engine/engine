/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/string.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    static constexpr ice::StringID Scheme_URN = "urn"_sid;
    static constexpr ice::StringID Scheme_File = "file"_sid;
    static constexpr ice::StringID Scheme_Dynlib = "dynlib"_sid;
    static constexpr ice::StringID Scheme_HailStorm = "hsc"_sid;

    static constexpr ice::StringID Scheme_Invalid = "<invalid>"_sid;

    struct URI
    {
        constexpr explicit URI(char const* uri_raw) noexcept;
        constexpr explicit URI(ice::String uri) noexcept;
        constexpr URI(ice::StringID_Arg scheme, ice::String uri) noexcept;

        constexpr URI(URI const& other) noexcept;
        constexpr auto operator=(URI const& other) noexcept -> ice::URI&;

        constexpr auto scheme() const noexcept -> ice::StringID;

        constexpr auto path() const noexcept -> ice::String;
        constexpr auto query() const noexcept -> ice::String;
        constexpr auto fragment() const noexcept -> ice::String;

        // Authority
        constexpr auto authority() const noexcept -> ice::String;
        constexpr auto userinfo() const noexcept -> ice::String;
        constexpr auto host() const noexcept -> ice::String;
        constexpr auto port() const noexcept -> ice::String;

        char const* _uri;
        ice::u8 _forced_scheme : 4;
        ice::u8 _scheme : 4;
        ice::u8 _authority;
        ice::u8 _userinfo;
        ice::u8 _host;
        ice::u8 _port;
        ice::u8 _path;
        ice::u8 _query;
        ice::u8 _fragment;
    };


    namespace detail
    {

        static constexpr ice::StringID Constant_KnownSchemes[]{
            Scheme_URN,
            Scheme_File,
            Scheme_HailStorm,
            Scheme_Dynlib,
            Scheme_Invalid
        };

        static_assert(ice::count(Constant_KnownSchemes) <= 15);

        constexpr bool get_scheme_size(ice::String raw_uri, ice::u8& out_size) noexcept
        {
            ice::ucount const scheme_end = ice::string::find_first_of(raw_uri, ':');
            if (scheme_end != ice::String_NPos)
            {
                out_size = static_cast<ice::u8>(scheme_end + 1);
            }
            return scheme_end != ice::String_NPos;
        }

        constexpr bool get_authority_sizes(
            ice::String uri,
            ice::u8& out_authority,
            ice::u8& out_user,
            ice::u8& out_host,
            ice::u8& out_port
        ) noexcept
        {
            if (uri[0] == '/' && uri[1] == '/')
            {
                ice::ucount const authority_end = ice::string::find_first_of(uri, '/', 2);
                if (authority_end == ice::String_NPos)
                {
                    if (std::is_constant_evaluated())
                    {
                        throw "Invalid URI definition! Do not use '//' without defining a host name!";
                    }
                    return false;
                }

                out_authority = static_cast<ice::u8>(authority_end);

                ice::ucount offset = 0;
                ice::String const authority_uri = ice::string::substr(uri, 2, authority_end - 2);
                ice::ucount const authority_user = ice::string::find_first_of(authority_uri, '@', offset);
                if (authority_user != ice::String_NPos)
                {
                    // Include the '@' character in the size, since it can be easily removed in the 'userinfo()' method
                    //  and helps with calculations.
                    offset = out_user = static_cast<ice::u8>(authority_user - offset) + 1;
                }

                ice::ucount const authority_port = ice::string::find_first_of(authority_uri, ':', offset);
                if (authority_port != ice::String_NPos)
                {
                    // If we have a port set the length of host to up to the ':' character
                    out_host = static_cast<ice::u8>(authority_port - offset);

                    // After that it's the 'port' value (without the ':') character
                    //  Because the host needs to exist we can always add +1, and keep the port size the actual size.
                    out_port = static_cast<ice::u8>(ice::size(authority_uri) - (authority_port + 1));
                }
                else
                {
                    // The rest of the authority string is the host
                    out_host = static_cast<ice::u8>(ice::size(authority_uri) - offset);
                }
            }
            return true;
        }

        constexpr bool get_path_query_fragment_sizes(
            ice::String uri,
            ice::u8& out_path,
            ice::u8& out_query,
            ice::u8& out_fragment
        ) noexcept
        {
            ice::ucount path_separator = ice::string::find_first_of(uri, ice::String{ "?#" });
            if (path_separator == ice::String_NPos)
            {
                out_path = static_cast<ice::u8>(ice::size(uri));
                return out_path > 0;
            }

            // We continue after assigning path length
            out_path = static_cast<ice::u8>(path_separator);

            // Do we have a query?
            if (uri[path_separator] == '?')
            {
                // Get the next separator if necessary
                path_separator = ice::string::find_last_of(uri, '#');
                if (path_separator == ice::String_NPos)
                {
                    // We take te remaining query with the starting '?' character
                    out_query = static_cast<ice::u8>(ice::size(uri) - out_path);
                    return true;
                }

                // The everything up to '#' including the '?' character.
                out_query = static_cast<ice::u8>(path_separator - out_path);
            }

            // Take everything remaining including the '#' character.
            out_fragment = static_cast<ice::u8>(ice::size(uri) - path_separator);
            return true;
        }

    } // namespace detail

    constexpr URI::URI(char const* uri_raw) noexcept
        : URI{ ice::String{ uri_raw } }
    {
    }

    constexpr URI::URI(ice::String uri_raw) noexcept
        : _uri{ ice::string::begin(uri_raw) }
        , _forced_scheme{ }
        , _scheme{ }
        , _authority{ }
        , _userinfo{ }
        , _host{ }
        , _port{ }
        , _path{ }
        , _query{ }
        , _fragment{ }
    {
        ice::u8 scheme_size;
        if (detail::get_scheme_size(uri_raw, scheme_size))
        {
            _scheme = scheme_size;
        }
        detail::get_authority_sizes(
            ice::string::substr(uri_raw, _scheme),
            _authority, _userinfo, _host, _port
        );
        detail::get_path_query_fragment_sizes(
            ice::string::substr(uri_raw, _scheme + _authority),
            _path, _query, _fragment
        );
    }

    constexpr URI::URI(ice::StringID_Arg forced_scheme, ice::String uri_raw) noexcept
        : URI{ uri_raw }
    {
        ice::u8 scheme_idx = 0;
        for (ice::StringID_Arg scheme : detail::Constant_KnownSchemes)
        {
            if (scheme == forced_scheme)
            {
                break;
            }
            scheme_idx += 1;
        }
        _forced_scheme = scheme_idx;
        _scheme = 0;
    }

    constexpr URI::URI(URI const& other) noexcept
        : _uri{ other._uri }
        , _forced_scheme{ other._forced_scheme }
        , _scheme{ other._scheme }
        , _authority{ other._authority }
        , _userinfo{ other._userinfo }
        , _host{ other._host }
        , _port{ other._port }
        , _path{ other._path }
        , _query{ other._query }
        , _fragment{ other._fragment }
    {
    }

    constexpr auto URI::operator=(URI const& other) noexcept -> ice::URI&
    {
        if (ice::addressof(other) != this)
        {
            _uri = other._uri;
            _forced_scheme = other._forced_scheme;
            _scheme = other._scheme;
            _authority = other._authority;
            _userinfo = other._userinfo;
            _host = other._host;
            _port = other._port;
            _path = other._path;
            _query = other._query;
            _fragment = other._fragment;
        }
        return *this;
    }

    constexpr auto URI::scheme() const noexcept -> ice::StringID
    {
        // Always return URN scheme by default
        return _scheme == 0
            ? detail::Constant_KnownSchemes[_forced_scheme]
            : ice::stringid(ice::String{_uri, ice::u32(_scheme - 1)});
    }

    constexpr auto URI::path() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + _authority, _path };
    }

    constexpr auto URI::query() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + _authority + _path, _query };
    }

    constexpr auto URI::fragment() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + _authority + _path + _query, _fragment };
    }

    // Authority
    constexpr auto URI::authority() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme, _authority };
    }

    constexpr auto URI::userinfo() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + 2 /* removes '//' */, ice::ucount(_userinfo - (_userinfo != 0)) };
    }

    constexpr auto URI::host() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + 2 + _userinfo, _host };
    }

    constexpr auto URI::port() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + 2 + _userinfo + _host + 1  /* removes ':' at the start */, _port };
    }

    // Helpers
    constexpr auto operator""_uri(char const* raw_uri, std::size_t length) noexcept -> ice::URI
    {
        return URI{ ice::String{ raw_uri, ice::ucount(length) } };
    }

} // namespace ice
