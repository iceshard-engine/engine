/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/math.hxx>

static ice::f32 constexpr test_value_1 = 1.11f;
static ice::f32 constexpr test_value_2 = -2.22f;
static ice::f32 constexpr test_value_3 = 15.999f;
static ice::f32 constexpr test_value_4 = 1.f;

TEST_CASE("math 'ice/math.hxx'", "[math]")
{
    SECTION("Type traits")
    {
        CHECK(ice::vec1f::count_rows == 1);
        CHECK(ice::vec1f::count_columns == 1);
    }

    SECTION("Initialization")
    {
        ice::vec1f value1{ test_value_1 };
        ice::vec1f value2{ test_value_2 };

        CHECK(value1.x == test_value_1);
        CHECK(value2.x == test_value_2);

        CHECK(value1.v[0][0] == test_value_1);
        CHECK(value2.v[0][0] == test_value_2);
    }

    SECTION("Operations (explicit)")
    {
        ice::vec1f value1{ test_value_1 };
        ice::vec1f value2{ test_value_2 };

        SECTION("Add")
        {
            ice::vec1f result = ice::add(value1, value2);

            CHECK(result.x == (test_value_1 + test_value_2));
            CHECK(result.v[0][0] == (test_value_1 + test_value_2));
        }

        SECTION("Subtract")
        {
            ice::vec1f result = ice::sub(value1, value2);

            CHECK(result.x == (test_value_1 - test_value_2));
            CHECK(result.v[0][0] == (test_value_1 - test_value_2));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec1f result = ice::mul(value2, test_value_1);

            CHECK(result.x == (test_value_2 * test_value_1));
            CHECK(result.v[0][0] == (test_value_2 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec1f result = ice::div(value2, test_value_1);

            CHECK(result.x == (test_value_2 / test_value_1));
            CHECK(result.v[0][0] == (test_value_2 / test_value_1));
        }
    }

    SECTION("Operators (implicit)")
    {
        ice::vec1f value1{ test_value_1 };
        ice::vec1f value2{ test_value_2 };

        SECTION("Add")
        {
            ice::vec1f result = value1 + value2;

            CHECK(result.x == (test_value_1 + test_value_2));
            CHECK(result.v[0][0] == (test_value_1 + test_value_2));
        }

        SECTION("Subtract")
        {
            ice::vec1f result = value1 - value2;

            CHECK(result.x == (test_value_1 - test_value_2));
            CHECK(result.v[0][0] == (test_value_1 - test_value_2));
        }
    }
}

TEST_CASE("Vec2 :: f32", "[math]")
{
    SECTION("Type traits")
    {
        CHECK(ice::vec2f::count_rows == 2);
        CHECK(ice::vec2f::count_columns == 1);
    }

    SECTION("Initialization")
    {
        ice::vec2f value1{ test_value_1 };
        ice::vec2f value2{ test_value_1, test_value_2 };

        CHECK(value1.x == test_value_1);
        CHECK(value1.y == test_value_1);
        CHECK(value2.x == test_value_1);
        CHECK(value2.y == test_value_2);

        CHECK(value1.v[0][0] == test_value_1);
        CHECK(value1.v[0][1] == test_value_1);
        CHECK(value2.v[0][0] == test_value_1);
        CHECK(value2.v[0][1] == test_value_2);
    }

    SECTION("Operations (explicit)")
    {
        ice::vec2f value1{ test_value_1 };
        ice::vec2f value2{ test_value_1, test_value_2 };

        SECTION("Add")
        {
            ice::vec2f result = ice::add(value1, value2);

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
        }

        SECTION("Subtract")
        {
            ice::vec2f result = ice::sub(value1, value2);

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec2f result = ice::mul(value2, test_value_1);

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec2f result = ice::div(value2, test_value_1);

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
        }
    }

    SECTION("Operators (implicit)")
    {
        ice::vec2f value1{ test_value_1 };
        ice::vec2f value2{ test_value_1, test_value_2 };

        SECTION("Negate")
        {
            ice::vec2f result = -value1;

            CHECK(result.x == -test_value_1);
            CHECK(result.y == -test_value_1);
            CHECK(result.v[0][0] == -test_value_1);
            CHECK(result.v[0][1] == -test_value_1);
        }

        SECTION("Add")
        {
            ice::vec2f result = value1 + value2;

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
        }

        SECTION("Subtract")
        {
            ice::vec2f result = value1 - value2;

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec2f result = value2 * test_value_1;

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec2f result = value2 / test_value_1;

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
        }
    }
}

TEST_CASE("Vec3 :: f32", "[math]")
{
    SECTION("Type traits")
    {
        CHECK(ice::vec3f::count_rows == 3);
        CHECK(ice::vec3f::count_columns == 1);
    }

    SECTION("Initialization")
    {
        ice::vec3f value1{ test_value_1 };
        ice::vec3f value2{ test_value_1, test_value_2, test_value_3 };

        CHECK(value1.x == test_value_1);
        CHECK(value1.y == test_value_1);
        CHECK(value1.z == test_value_1);
        CHECK(value2.x == test_value_1);
        CHECK(value2.y == test_value_2);
        CHECK(value2.z == test_value_3);

        CHECK(value1.v[0][0] == test_value_1);
        CHECK(value1.v[0][1] == test_value_1);
        CHECK(value1.v[0][2] == test_value_1);
        CHECK(value2.v[0][0] == test_value_1);
        CHECK(value2.v[0][1] == test_value_2);
        CHECK(value2.v[0][2] == test_value_3);
    }

    SECTION("Operations (explicit)")
    {
        ice::vec3f value1{ test_value_1 };
        ice::vec3f value2{ test_value_1, test_value_2, test_value_3 };

        SECTION("Add")
        {
            ice::vec3f result = ice::add(value1, value2);

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.z == (test_value_1 + test_value_3));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
            CHECK(result.v[0][2] == (test_value_1 + test_value_3));
        }

        SECTION("Subtract")
        {
            ice::vec3f result = ice::sub(value1, value2);

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.z == (test_value_1 - test_value_3));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
            CHECK(result.v[0][2] == (test_value_1 - test_value_3));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec3f result = ice::mul(value2, test_value_1);

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.z == (test_value_3 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
            CHECK(result.v[0][2] == (test_value_3 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec3f result = ice::div(value2, test_value_1);

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.z == (test_value_3 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
            CHECK(result.v[0][2] == (test_value_3 / test_value_1));
        }

        SECTION("Cross")
        {
            ice::vec3f result = ice::cross(value1, value2);

            ice::f32 const v_x = test_value_1 * test_value_3 - test_value_2 * test_value_1;
            ice::f32 const v_y = test_value_1 * test_value_1 - test_value_3 * test_value_1;
            ice::f32 const v_z = test_value_1 * test_value_2 - test_value_1 * test_value_1;

            CHECK(result.x == v_x);
            CHECK(result.y == v_y);
            CHECK(result.z == v_z);
            CHECK(result.v[0][0] == v_x);
            CHECK(result.v[0][1] == v_y);
            CHECK(result.v[0][2] == v_z);
        }

        SECTION("Dot")
        {
            ice::f32 result = ice::dot(value1, value2);

            ice::f32 const v_x = test_value_1 * test_value_1;
            ice::f32 const v_y = test_value_1 * test_value_2;
            ice::f32 const v_z = test_value_1 * test_value_3;

            CHECK(result == (v_x + v_y + v_z));
        }

        SECTION("Normalize")
        {
            ice::f32 const sqrt_inverted = 1.f / ice::sqrt(ice::dot(value2, value2));

            ice::vec3f result = ice::normalize(value2);

            CHECK(result.x == (test_value_1 * sqrt_inverted));
            CHECK(result.y == (test_value_2 * sqrt_inverted));
            CHECK(result.z == (test_value_3 * sqrt_inverted));
            CHECK(result.v[0][0] == (test_value_1 * sqrt_inverted));
            CHECK(result.v[0][1] == (test_value_2 * sqrt_inverted));
            CHECK(result.v[0][2] == (test_value_3 * sqrt_inverted));
        }
    }

    SECTION("Operators (implicit)")
    {
        ice::vec3f value1{ test_value_1 };
        ice::vec3f value2{ test_value_1, test_value_2, test_value_3 };

        SECTION("Negate")
        {
            ice::vec3f result = -value1;

            CHECK(result.x == -test_value_1);
            CHECK(result.y == -test_value_1);
            CHECK(result.z == -test_value_1);
            CHECK(result.v[0][0] == -test_value_1);
            CHECK(result.v[0][1] == -test_value_1);
            CHECK(result.v[0][2] == -test_value_1);
        }

        SECTION("Add")
        {
            ice::vec3f result = value1 + value2;

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.z == (test_value_1 + test_value_3));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
            CHECK(result.v[0][2] == (test_value_1 + test_value_3));
        }

        SECTION("Subtract")
        {
            ice::vec3f result = value1 - value2;

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.z == (test_value_1 - test_value_3));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
            CHECK(result.v[0][2] == (test_value_1 - test_value_3));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec3f result = value2 * test_value_1;

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.z == (test_value_3 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
            CHECK(result.v[0][2] == (test_value_3 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec3f result = value2 / test_value_1;

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.z == (test_value_3 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
            CHECK(result.v[0][2] == (test_value_3 / test_value_1));
        }
    }
}

TEST_CASE("Vec4 :: f32", "[math]")
{
    SECTION("Type traits")
    {
        CHECK(ice::vec4f::count_rows == 4);
        CHECK(ice::vec4f::count_columns == 1);
    }

    SECTION("Initialization")
    {
        ice::vec4f value1{ test_value_1 };
        ice::vec4f value2{ test_value_1, test_value_2, test_value_3, test_value_4 };

        CHECK(value1.x == test_value_1);
        CHECK(value1.y == test_value_1);
        CHECK(value1.z == test_value_1);
        CHECK(value1.w == test_value_1);
        CHECK(value2.x == test_value_1);
        CHECK(value2.y == test_value_2);
        CHECK(value2.z == test_value_3);
        CHECK(value2.w == test_value_4);

        CHECK(value1.v[0][0] == test_value_1);
        CHECK(value1.v[0][1] == test_value_1);
        CHECK(value1.v[0][2] == test_value_1);
        CHECK(value1.v[0][3] == test_value_1);
        CHECK(value2.v[0][0] == test_value_1);
        CHECK(value2.v[0][1] == test_value_2);
        CHECK(value2.v[0][2] == test_value_3);
        CHECK(value2.v[0][3] == test_value_4);
    }

    SECTION("Operations (explicit)")
    {
        ice::vec4f value1{ test_value_1 };
        ice::vec4f value2{ test_value_1, test_value_2, test_value_3, test_value_4 };

        SECTION("Add")
        {
            ice::vec4f result = ice::add(value1, value2);

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.z == (test_value_1 + test_value_3));
            CHECK(result.w == (test_value_1 + test_value_4));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
            CHECK(result.v[0][2] == (test_value_1 + test_value_3));
            CHECK(result.v[0][3] == (test_value_1 + test_value_4));
        }

        SECTION("Subtract")
        {
            ice::vec4f result = ice::sub(value1, value2);

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.z == (test_value_1 - test_value_3));
            CHECK(result.w == (test_value_1 - test_value_4));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
            CHECK(result.v[0][2] == (test_value_1 - test_value_3));
            CHECK(result.v[0][3] == (test_value_1 - test_value_4));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec4f result = ice::mul(value2, test_value_1);

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.z == (test_value_3 * test_value_1));
            CHECK(result.w == (test_value_4 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
            CHECK(result.v[0][2] == (test_value_3 * test_value_1));
            CHECK(result.v[0][3] == (test_value_4 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec4f result = ice::div(value2, test_value_1);

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.z == (test_value_3 / test_value_1));
            CHECK(result.w == (test_value_4 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
            CHECK(result.v[0][2] == (test_value_3 / test_value_1));
            CHECK(result.v[0][3] == (test_value_4 / test_value_1));
        }
    }

    SECTION("Operators (implicit)")
    {
        ice::vec4f value1{ test_value_1 };
        ice::vec4f value2{ test_value_1, test_value_2, test_value_3, test_value_4 };

        SECTION("Negate")
        {
            ice::vec4f result = -value1;

            CHECK(result.x == -test_value_1);
            CHECK(result.y == -test_value_1);
            CHECK(result.z == -test_value_1);
            CHECK(result.w == -test_value_1);
            CHECK(result.v[0][0] == -test_value_1);
            CHECK(result.v[0][1] == -test_value_1);
            CHECK(result.v[0][2] == -test_value_1);
            CHECK(result.v[0][3] == -test_value_1);
        }

        SECTION("Add")
        {
            ice::vec4f result = value1 + value2;

            CHECK(result.x == (test_value_1 + test_value_1));
            CHECK(result.y == (test_value_1 + test_value_2));
            CHECK(result.z == (test_value_1 + test_value_3));
            CHECK(result.w == (test_value_1 + test_value_4));
            CHECK(result.v[0][0] == (test_value_1 + test_value_1));
            CHECK(result.v[0][1] == (test_value_1 + test_value_2));
            CHECK(result.v[0][2] == (test_value_1 + test_value_3));
            CHECK(result.v[0][3] == (test_value_1 + test_value_4));
        }

        SECTION("Subtract")
        {
            ice::vec4f result = value1 - value2;

            CHECK(result.x == (test_value_1 - test_value_1));
            CHECK(result.y == (test_value_1 - test_value_2));
            CHECK(result.z == (test_value_1 - test_value_3));
            CHECK(result.w == (test_value_1 - test_value_4));
            CHECK(result.v[0][0] == (test_value_1 - test_value_1));
            CHECK(result.v[0][1] == (test_value_1 - test_value_2));
            CHECK(result.v[0][2] == (test_value_1 - test_value_3));
            CHECK(result.v[0][3] == (test_value_1 - test_value_4));
        }

        SECTION("Multiply (by scalar)")
        {
            ice::vec4f result = value2 * test_value_1;

            CHECK(result.x == (test_value_1 * test_value_1));
            CHECK(result.y == (test_value_2 * test_value_1));
            CHECK(result.z == (test_value_3 * test_value_1));
            CHECK(result.w == (test_value_4 * test_value_1));
            CHECK(result.v[0][0] == (test_value_1 * test_value_1));
            CHECK(result.v[0][1] == (test_value_2 * test_value_1));
            CHECK(result.v[0][2] == (test_value_3 * test_value_1));
            CHECK(result.v[0][3] == (test_value_4 * test_value_1));
        }

        SECTION("Divide (by scalar)")
        {
            ice::vec4f result = value2 / test_value_1;

            CHECK(result.x == (test_value_1 / test_value_1));
            CHECK(result.y == (test_value_2 / test_value_1));
            CHECK(result.z == (test_value_3 / test_value_1));
            CHECK(result.w == (test_value_4 / test_value_1));
            CHECK(result.v[0][0] == (test_value_1 / test_value_1));
            CHECK(result.v[0][1] == (test_value_2 / test_value_1));
            CHECK(result.v[0][2] == (test_value_3 / test_value_1));
            CHECK(result.v[0][3] == (test_value_4 / test_value_1));
        }
    }
}
