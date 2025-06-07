/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>

namespace ice::info
{

    //! \brief Name of the engine.
    static constexpr char const engine_name[] = "iceshard";

    //! \brief Engine version the codebase was based on. (as a string)
    static constexpr char const engine_version_str[] = "0.5.0";

    //! \brief Engine version the codebase was based on.
    static constexpr ice::u8 engine_version_major = 0;
    static constexpr ice::u8 engine_version_minor = 5;
    static constexpr ice::u8 engine_version_patch = 0;

    //! \brief Stability category of this engine version.
    //! \note The following descriptions apply:
    //!   - Alpha - Might be unstable or crash at times.
    //!   - Beta - Overall stable but in some cornercases instability might still occur.
    //!   - RC - The build doesn't crash, has stable performance and all critical known issues are fixed.
    //!   - Review - The build is open to the public for review / early-adoption.
    //!   - Public - The build is considered stable and ready for use by the general public.
    enum class Stability { Alpha, Beta, RC, Review, Public };

    //! \brief Engine stability of the current version.
    static constexpr Stability stability = Stability::Alpha;

} // namespace ice
