/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string.hxx>

namespace ice::uri
{

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
        if (path_separator == nindex_none)
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
            if (path_separator == nindex_none)
            {
                // We take te remaining query with the starting '?' character
                out_query = (uri.size() - out_path).u8();
                return true;
            }

            // Take everything up to '#' including the '?' character.
            out_query = (path_separator - out_path).u8();
        }
        else
        {
            // URI specification does not support queries after the '#fragment'
            ICE_ASSERT_CORE(uri.find_first_of('?', path_separator + 1) == ice::nindex_none);
        }

        // Take everything remaining including the '#' character.
        out_fragment = (uri.size() - path_separator).u8();
        return true;
    }

} // namespace ice::uri
