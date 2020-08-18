#include <core/clock.hxx>
#include <core/platform/windows.hxx>

namespace core::clock
{

    auto core::clock::create_clock() noexcept -> Clock<>
    {
        LARGE_INTEGER large_int;
        QueryPerformanceFrequency(&large_int);

        return Clock<>{
            .frequency = large_int.QuadPart
        };
    }

    template<>
    void update(Clock<>& c) noexcept
    {
        LARGE_INTEGER large_int;
        QueryPerformanceCounter(&large_int);

        c.previous_timestamp = c.latest_timestamp;
        c.latest_timestamp = large_int.QuadPart;
    }

} // namespace core::clock
