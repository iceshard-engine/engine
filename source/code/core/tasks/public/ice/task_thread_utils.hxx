#pragma once
#include <ice/clock_types.hxx>

namespace ice::current_thread
{

    //! \brief Sleep for the selecter number of milliseconds.
    void sleep(Tms ms) noexcept;

} // namespace ice::current_thread
