#pragma once
#include <core/datetime/types.hxx>

namespace core::datetime::constant
{

static constexpr tick_type ticks_per_microsecond{ 10 };

static constexpr tick_type ticks_per_millisecond{ ticks_per_microsecond.value * 1000 };

static constexpr tick_type ticks_per_second{ ticks_per_millisecond.value * 1000 };

static constexpr tick_type ticks_per_minute{ ticks_per_second.value * 60 };

static constexpr tick_type ticks_per_hour{ ticks_per_minute.value * 60 };

static constexpr tick_type ticks_per_day{ ticks_per_hour.value * 24 };


static constexpr int32_t days_per_year{ 365 };

static constexpr int32_t days_per_4years{ days_per_year * 4 + 1 };

static constexpr int32_t days_per_100years{ days_per_4years * 25 - 1 };

static constexpr int32_t days_per_400years{ days_per_100years * 4 + 1 };


static constexpr int32_t days_to_1601{ days_per_400years * 4 };

static constexpr int32_t days_to_1899{ days_per_400years * 4 + days_per_100years * 3 - 367 };

static constexpr int32_t days_to_1970{ days_per_400years * 4 + days_per_4years * 17 + days_per_year };

static constexpr int32_t days_to_10000{ days_per_400years * 24 - 366 };


static constexpr tick_type ticks_to_1601{ days_to_1601 * ticks_per_day.value };

static constexpr tick_type ticks_to_1899{ days_to_1899 * ticks_per_day.value };

static constexpr tick_type ticks_to_1970{ days_to_1970 * ticks_per_day.value };


static constexpr tick_type min_ticks{ 0 };

static constexpr tick_type max_ticks{ days_to_10000 * ticks_per_day.value - 1 };

} // namespace core::datetime::constant
