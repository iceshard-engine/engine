/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>
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
        constexpr URI() noexcept;
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

        constexpr bool get_known_scheme_index(ice::StringID_Arg known_scheme) noexcept
        {
            ice::u8 scheme_idx = 0;
            for (ice::StringID_Arg scheme : detail::Constant_KnownSchemes)
            {
                if (scheme == known_scheme)
                {
                    break;
                }
                scheme_idx += 1;
            }
            return scheme_idx;
        }

        constexpr bool get_scheme_size(ice::String raw_uri, ice::u8& out_size) noexcept
        {
            ice::nindex const scheme_end = raw_uri.find_first_of(':');
            if (scheme_end.is_valid())
            {
                out_size = scheme_end.u8() + 1;
            }
            return scheme_end.is_valid();
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
                ice::nindex const authority_end = uri.find_first_of('/', 2);
                ICE_ASSERT_CORE(authority_end.is_valid());
                if (authority_end.is_valid() == false)
                {
                    return false;
                }

                out_authority = authority_end.u8();

                ice::nindex offset = 0;
                ice::String const authority_uri = uri.substr(2, authority_end.u32() - 2);
                ice::nindex const authority_user = authority_uri.find_first_of('@', offset);
                if (authority_user.is_valid())
                {
                    // Include the '@' character in the size, since it can be easily removed in the 'userinfo()' method
                    //  and helps with calculations.
                    offset = out_user = (authority_user - offset).u8() + 1;
                }

                ice::nindex const authority_port = authority_uri.find_first_of(':', offset);
                if (authority_port.is_valid())
                {
                    // If we have a port set the length of host to up to the ':' character
                    out_host = (authority_port - offset).u8();

                    // After that it's the 'port' value (without the ':') character
                    //  Because the host needs to exist we can always add +1, and keep the port size the actual size.
                    out_port = (authority_uri.size() - (authority_port + 1)).u8();
                }
                else
                {
                    // The rest of the authority string is the host
                    out_host = (authority_uri.size() - offset).u8();
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
            ice::nindex path_separator = uri.find_first_of("?#");
            if (path_separator == none_index)
            {
                out_path = uri.size().u8();
                return out_path > 0;
            }

            // We continue after assigning path length
            out_path = path_separator.u8();

            // Do we have a query?
            if (uri[path_separator] == '?')
            {
                // Get the next separator if necessary
                path_separator = uri.find_last_of('#');
                if (path_separator == none_index)
                {
                    // We take te remaining query with the starting '?' character
                    out_query = (uri.size() - out_path).u8();
                    return true;
                }

                // The everything up to '#' including the '?' character.
                out_query = (path_separator - out_path).u8();
            }

            // Take everything remaining including the '#' character.
            out_fragment = (uri.size() - path_separator).u8();
            return true;
        }

    } // namespace detail

    constexpr URI::URI() noexcept
        : _uri{ nullptr }
        , _forced_scheme{ detail::get_known_scheme_index(Scheme_Invalid) }
        , _scheme{ }
        , _authority{ }
        , _userinfo{ }
        , _host{ }
        , _port{ }
        , _path{ }
        , _query{ }
        , _fragment{ }
    {
    }

    constexpr URI::URI(char const* uri_raw) noexcept
        : URI{ ice::String{ uri_raw } }
    {
    }

    constexpr URI::URI(ice::String uri_raw) noexcept
        : _uri{ uri_raw.begin() }
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
        ice::u8 scheme_size = 0;
        if (detail::get_scheme_size(uri_raw, scheme_size))
        {
            _scheme = scheme_size;
        }
        detail::get_authority_sizes(
            uri_raw.substr(_scheme),
            _authority, _userinfo, _host, _port
        );
        detail::get_path_query_fragment_sizes(
            uri_raw.substr(_scheme + _authority),
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
        return ice::String{ _uri + _scheme + 2 /* removes '//' */, ice::u32(_userinfo - (_userinfo != 0)) };
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
        return URI{ ice::String{ raw_uri, length } };
    }

} // namespace ice
