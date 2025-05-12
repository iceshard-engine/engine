/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#include <catch2/catch_test_macros.hpp>
#include <ice/config.hxx>
#include <ice/config/config_builder.hxx>
#include <ice/mem_allocator_host.hxx>

SCENARIO("utils `ice/config.hxx` | type-integrity", "[utils][config][type_integrity]")
{
    ice::HostAllocator alloc;

    // Create a builder and fill it with various data values
    ice::ConfigBuilder builder{ alloc };

    GIVEN("various number types...")
    {
        ice::ConfigBuilderValue numbers = builder["numbers"];
        numbers["u8"] = ice::u8{ 234 };
        numbers["u16"] = ice::u16{ 62'421 };
        numbers["u32"] = ice::u32{ 12'860'112 };
        numbers["u64"] = ice::u64{ 982'567'112'860'112 };

        numbers["i8"] = ice::i8{ -116 };
        numbers["i16"] = ice::i16{ -31'421 };
        numbers["i32"] = ice::i32{ -12'860'112 };
        numbers["i64"] = ice::i64{ -982'567'112'860'112 };

        numbers["f32"] = ice::f32{ -7.21123f };
        numbers["f64"] = ice::f64{ 11.21123998212356234 };

        ice::Memory const configmem =  builder.finalize(alloc);
        ice::Config const config = ice::config::from_data(ice::data_view(configmem));

        THEN("we can extract them with their exact types...")
        {
            CHECK(ice::config::get<ice::u8>(config, "numbers.u8").value() == ice::u8{ 234 });
            CHECK(ice::config::get<ice::u16>(config, "numbers.u16").value() == ice::u16{ 62'421 });
            CHECK(ice::config::get<ice::u32>(config, "numbers.u32").value() == ice::u32{ 12'860'112 });
            CHECK(ice::config::get<ice::u64>(config, "numbers.u64").value() == ice::u64{ 982'567'112'860'112 });

            CHECK(ice::config::get<ice::i8>(config, "numbers.i8").value() == ice::i8{ -116 });
            CHECK(ice::config::get<ice::i16>(config, "numbers.i16").value() == ice::i16{ -31'421 });
            CHECK(ice::config::get<ice::i32>(config, "numbers.i32").value() == ice::i32{ -12'860'112 });
            CHECK(ice::config::get<ice::i64>(config, "numbers.i64").value() == ice::i64{ -982'567'112'860'112 });

            CHECK(ice::config::get<ice::f32>(config, "numbers.f32").value() == ice::f32{ -7.21123f });
            CHECK(ice::config::get<ice::f64>(config, "numbers.f64").value() == ice::f64{ 11.21123998212356234 });
        }

        THEN("we can extract them with casting (expand / truncate)...")
        {
            CHECK(ice::config::get<ice::u32>(config, "numbers.u8", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::u8{ 234 });
            CHECK(ice::config::get<ice::u32>(config, "numbers.u16", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::u16{ 62'421 });
            CHECK(ice::config::get<ice::u32>(config, "numbers.u32", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::u32{ 12'860'112 });
            // Explicit truncate
            CHECK(ice::config::get<ice::u32>(config, "numbers.u64", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::u32>(982'567'112'860'112));

            CHECK(ice::config::get<ice::i32>(config, "numbers.i8", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::i8{ -116 });
            CHECK(ice::config::get<ice::i32>(config, "numbers.i16", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::i16{ -31'421 });
            CHECK(ice::config::get<ice::i32>(config, "numbers.i32", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::i32{ -12'860'112 });
            // Explicit truncate
            CHECK(ice::config::get<ice::i32>(config, "numbers.i64", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::i32>(-982'567'112'860'112));

            CHECK(ice::config::get<ice::f32>(config, "numbers.f32", ice::ConfigValueFlags::AllowImplicitCasts).value() == ice::f32{ -7.21123f });
            // Explicit truncate
            CHECK(ice::config::get<ice::f32>(config, "numbers.f64", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::f32>(11.21123998212356234));
        }

        THEN("we can extract them with casting (conversion)...")
        {
            CHECK(ice::config::get<ice::i8>(config, "numbers.u8", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::i8>(234));
            CHECK(ice::config::get<ice::i16>(config, "numbers.u16", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::i16>(62'421));
            CHECK(ice::config::get<ice::i32>(config, "numbers.u32", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::i32>(12'860'112));
            CHECK(ice::config::get<ice::i64>(config, "numbers.u64", ice::ConfigValueFlags::AllowImplicitCasts).value() == static_cast<ice::i64>(982'567'112'860'112));

            CHECK(ice::config::get<ice::u8>(config, "numbers.i8", ice::ConfigValueFlags::AllowImplicitCasts).value() == 0);
            CHECK(ice::config::get<ice::u16>(config, "numbers.i16", ice::ConfigValueFlags::AllowImplicitCasts).value() == 0);
            CHECK(ice::config::get<ice::u32>(config, "numbers.i32", ice::ConfigValueFlags::AllowImplicitCasts).value() == 0);
            CHECK(ice::config::get<ice::u64>(config, "numbers.i64", ice::ConfigValueFlags::AllowImplicitCasts).value() == 0);

            CHECK(ice::config::get<ice::i32>(config, "numbers.f32", ice::ConfigValueFlags::AllowImplicitCasts).value() == -7);
            CHECK(ice::config::get<ice::u64>(config, "numbers.f64", ice::ConfigValueFlags::AllowImplicitCasts).value() == 11);
        }

        alloc.deallocate(configmem);
    }
}
