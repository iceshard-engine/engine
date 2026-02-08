/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <fmt/format.h>
#include <catch2/catch_test_macros.hpp>

struct Test_ObjectEvents
{
    ice::u32 test_ctor;
    ice::u32 test_ctor_def;
    ice::u32 test_ctor_move;
    ice::u32 test_ctor_copy;

    ice::u32 test_op_move;
    ice::u32 test_op_copy;

    ice::u32* test_dtor = nullptr;
};

bool operator==(Test_ObjectEvents const& lhs, Test_ObjectEvents const& rhs) noexcept;

struct Test_TrackingObject
{
    ice::u32 value;

    Test_ObjectEvents data;

    Test_TrackingObject() noexcept;
    Test_TrackingObject(ice::u32 value) noexcept;
    Test_TrackingObject(Test_TrackingObject&&) noexcept;
    Test_TrackingObject(Test_TrackingObject const&) noexcept;
    ~Test_TrackingObject() noexcept;

    auto operator=(Test_TrackingObject&&) noexcept -> Test_TrackingObject&;
    auto operator=(Test_TrackingObject const&) noexcept -> Test_TrackingObject&;

    void gather_ctors(Test_ObjectEvents& events) const noexcept;
    void gather_operators(Test_ObjectEvents& events) const noexcept;

    bool operator==(Test_TrackingObject const& obj) const noexcept;
    bool operator==(Test_ObjectEvents const& events) const noexcept;
};

namespace Catch
{

    template<>
    struct StringMaker<Test_ObjectEvents>
    {
        static std::string convert(Test_ObjectEvents const& value)
        {
            return fmt::format(
                "({}) (... {}) (const& {}) (&& {}) ~({}) =(const& {}) =(&& {})",
                value.test_ctor,
                value.test_ctor_def,
                value.test_ctor_move,
                value.test_ctor_copy,
                value.test_dtor ? *value.test_dtor : 0,
                value.test_op_move,
                value.test_op_copy
            );
        }
    };

    template<>
    struct StringMaker<Test_TrackingObject>
    {
        static std::string convert(Test_TrackingObject const& value)
        {
            return StringMaker<Test_ObjectEvents>::convert(value.data);
        }
    };

} // namespace Catch
