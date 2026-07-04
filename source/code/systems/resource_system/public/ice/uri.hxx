/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>
#include <ice/path_utils.hxx>
#include <ice/uri_tools.hxx>

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

        constexpr auto path() const noexcept -> ice::Path;
        constexpr auto query() const noexcept -> ice::String;
        constexpr auto fragment() const noexcept -> ice::String;

        // Query Parsing
        constexpr auto parameter(ice::String name, ice::String fallback_value) const noexcept -> ice::String;

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
        if (ice::uri::get_scheme_size(uri_raw, scheme_size))
        {
            _scheme = scheme_size;
        }
        ice::uri::get_authority_sizes(
            uri_raw.substr(_scheme),
            _authority, _userinfo, _host, _port
        );
        ice::uri::get_path_query_fragment_sizes(
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

    constexpr auto URI::path() const noexcept -> ice::Path
    {
        return ice::Path{ _uri + _scheme + _authority, _path };
    }

    constexpr auto URI::query() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + _authority + _path, _query };
    }

    constexpr auto URI::fragment() const noexcept -> ice::String
    {
        return ice::String{ _uri + _scheme + _authority + _path + _query, _fragment };
    }

    constexpr auto URI::parameter(ice::String name, ice::String fallback_value) const noexcept -> ice::String
    {
        ice::String parameters = query().substr(1); // Remove the initial '?' query character
        if (parameters.is_empty())
        {
            return fallback_value;
        }

        ice::String result{};
        do
        {
            ice::nindex const param_sep = parameters.find_first_of('&');
            ice::String const param_keyval = parameters.substr(0, param_sep);
            ice::nindex const value_sep = param_keyval.find_first_of('=');
            if (param_keyval.substr(0, value_sep) == name)
            {
                result = param_keyval.substr(value_sep + 1);
                parameters = {};
            }
            else if (param_sep == nindex_none)
            {
                parameters = {};
            }
            else
            {
                parameters = parameters.substr(param_sep + 1);
            }
        }
        while (parameters.not_empty());

        return result;
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
