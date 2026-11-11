/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

#define ICE_WITH_DEVUI ICE_DEBUG || ICE_DEVELOP

namespace ice::build
{

    static constexpr bool with_devui = ice::build::is_debug || ice::build::is_develop;

} // namespace iceW