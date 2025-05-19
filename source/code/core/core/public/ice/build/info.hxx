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

} // namespace ice
