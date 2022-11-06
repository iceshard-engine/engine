/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once

namespace ice::build::validation
{

    /// \brief (ASCII) String value used to validate the use of '/utf-8' or '--utf8' compiler flags.
    static constexpr char const ascii_string[] = "hello";

    /// \brief (UTF8) String value used to validate the use of '/utf-8' or '--utf8' compiler flags.
    static constexpr char const utf8_string[] = "こんにちは";

    /// \brief (UTF8) String value used to validate built-in utf8 support.
    static constexpr char8_t const utf8_string_explicit[] = u8"こんにちは";

    // We test the string size to ensure ASCII values have expected size.
    static_assert(sizeof(ascii_string) == 6);

    // We test the string size to ensure UTF8 values have expected extended size.
    // Note: This assertion might be seen as error in some IDE's. But it will properly pass during compilation.
    static_assert(sizeof(utf8_string) == 16);

    // We test the string size to ensure UTF8 values have expected extended size.
    static_assert(sizeof(utf8_string_explicit) == 16);

} // ice::build::validation
