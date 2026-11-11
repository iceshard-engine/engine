/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/error_codes.hxx>

namespace ice
{

    static constexpr ice::ErrorCode E_AssetInvalidHandle{ "E.0300:Asset:Asset handle is invalid (null)." };
    static constexpr ice::ErrorCode E_AssetNotFound{ "E.0301:Asset:Requested asset was not found." };
    static constexpr ice::ErrorCode E_AssetDataNotFound_Raw{ "E.0310:Asset:Requested 'Raw' state data is not available." };
    static constexpr ice::ErrorCode E_AssetDataNotFound_Baked{ "E.0311:Asset:Requested 'Baked' state data is not available." };
    static constexpr ice::ErrorCode E_AssetDataNotFound_Loaded{ "E.0312:Asset:Requested 'Loaded' state data is not available." };
    static constexpr ice::ErrorCode E_AssetDataNotFound_Runtime{ "E.0313:Asset:Requested 'Runtime' state data is not available." };
    static constexpr ice::ErrorCode E_AssetDataNotFound[4]{ E_AssetDataNotFound_Raw, E_AssetDataNotFound_Baked, E_AssetDataNotFound_Loaded, E_AssetDataNotFound_Runtime };
    static constexpr ice::ErrorCode E_AssetInvalidObjectData{ "E.0320:Asset:Requested object type does not match the size / alignment of the returned data." };

} // namespace ice