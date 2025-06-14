/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/clock_types.hxx>

namespace ice::current_thread
{

    //! \brief Sleep for the selecter number of milliseconds.
    void sleep(Tms ms) noexcept;

} // namespace ice::current_thread
