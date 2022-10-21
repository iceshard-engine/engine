#include <catch2/catch.hpp>
#include <ice/mem_types.hxx>
#include <ice/mem_utils.hxx>
#include "test_utils.hxx"

SCENARIO("memsys 'ice/mem_types.hxx'", "[type_operators]")
{
    using namespace ice;

    GIVEN("two simple 'usize' values...")
    {
        static constexpr ice::isize::base_type size1_initial = 4;
        static constexpr ice::isize::base_type size2_initial = 32;

        static_assert(
            size1_initial < size2_initial,
            "Some of the test rely on these values having a specific relationship."
        );

        ice::usize size1 = { size1_initial };
        ice::usize size2 = { size2_initial };

        THEN("we can use arithmetic operators...")
        {
            // Addition
            CHECK(size1 + size2 == ice::usize{ size1_initial + size2_initial });
            CHECK(size2 + size1 == ice::usize{ size1_initial + size2_initial });
            CHECK(std::is_same_v<decltype(size1 + size2), ice::usize>);

            // Subtraciton
            CHECK(size1 - size2 == ice::isize{ size1_initial - size2_initial });
            CHECK(size2 - size1 == ice::isize{ size2_initial - size1_initial });
            CHECK(size2 - size1 == ice::usize{ size2_initial - size1_initial });
            CHECK(-size1 == ice::isize{ -size1_initial });
            CHECK(std::is_same_v<decltype(size1 - size2), ice::isize>);
            CHECK(std::is_same_v<decltype(-size2), ice::isize>);

            // Multiplication
            CHECK(size1 * size2 == ice::usize{ size1_initial * size2_initial });
            CHECK(size1 * 2 == ice::usize{ size1_initial * 2 });
            CHECK(std::is_same_v<decltype(size1 * size2), ice::usize>);

            // Division
            CHECK(size1 / size2 == ice::usize{ size1_initial / size2_initial });
            CHECK(size1 / 2 == ice::usize{ size1_initial / 2 });
            CHECK(std::is_same_v<decltype(size1 / size2), ice::usize>);
        }

        THEN("we can compare them...")
        {
            CHECK(size2 > size1);
            CHECK(size2 >= size1);
            CHECK(size1 < size2);
            CHECK(size1 <= size2);
            CHECK(size1 <= size1);
            CHECK(size2 >= size2);
            CHECK(size1 != size2);
            CHECK(size2 == size2);
        }

        THEN("we can reassing them...")
        {
            size1 = size2;
            size2 = 1_KiB;

            CHECK(size2 == ice::usize{ 1024 });
            CHECK(size1 == ice::usize{ size2_initial });
        }
    }

    GIVEN("two simple 'isize' values...")
    {
        static constexpr ice::isize::base_type size1_initial = 8;
        static constexpr ice::isize::base_type size2_initial = 16;

        static_assert(
            size1_initial < size2_initial,
            "Some of the test rely on these values having a specific relationship."
        );

        ice::isize size1 = { size1_initial };
        ice::isize size2 = { size2_initial };

        THEN("we can use arithmetic operators...")
        {
            // Addition
            CHECK(size1 + size2 == ice::isize{ size1_initial + size2_initial });
            CHECK(size2 + size1 == ice::isize{ size1_initial + size2_initial });
            CHECK(std::is_same_v<decltype(size1 + size2), ice::isize>);

            // Subtraciton
            CHECK(size1 - size2 == ice::isize{ size1_initial - size2_initial });
            CHECK(size2 - size1 == ice::isize{ size2_initial - size1_initial });
            CHECK(size2 - size1 == ice::isize{ size2_initial - size1_initial });
            CHECK(-size1 == ice::isize{ -size1_initial });
            CHECK(std::is_same_v<decltype(size1 - size2), ice::isize>);
            CHECK(std::is_same_v<decltype(-size2), ice::isize>);

            // Multiplication
            CHECK(size1 * size2 == ice::isize{ size1_initial * size2_initial });
            CHECK(size1 * 2 == ice::isize{ size1_initial * 2 });
            CHECK(std::is_same_v<decltype(size1* size2), ice::isize>);

            // Division
            CHECK(size1 / size2 == ice::isize{ size1_initial / size2_initial });
            CHECK(size1 / 2 == ice::isize{ size1_initial / 2 });
            CHECK(std::is_same_v<decltype(size1 / size2), ice::isize>);
        }

        THEN("we can compare them...")
        {
            CHECK(size2 > size1);
            CHECK(size2 >= size1);
            CHECK(size1 < size2);
            CHECK(size1 <= size2);
            CHECK(size1 <= size1);
            CHECK(size2 >= size2);
            CHECK(size1 != size2);
            CHECK(size2 == size2);
        }

        THEN("we can reassing them...")
        {
            size1 = size2;
            size2 = 1_KiB;

            CHECK(size2 == ice::isize{ 1024 });
            CHECK(size1 == ice::isize{ size2_initial });
        }
    }
}

SCENARIO("memsys 'ice/mem_utils.hxx'", "[alignment_logic]")
{
    using namespace ice;

    GIVEN("two simple 'usize' values...")
    {
        static constexpr ice::usize const size1 = 33_B;
        static constexpr ice::usize const size2 = 74_B;

        static_assert(size1.value % 2 == 1); // Assumptions of test cases about the given values.
        static_assert(size2.value % 2 == 0); // Assumptions of test cases about the given values.
        static_assert(size1.value < 128); // Assumptions of test cases about the given values.
        static_assert(size2.value < 128); // Assumptions of test cases about the given values.

        THEN("aligning values to '1' does nothing...")
        {
            CHECK(ice::align_to(size1, ice::ualign::b_1).padding == 0_B);
            CHECK(ice::align_to(size2, ice::ualign::b_1).padding == 0_B);

            CHECK(ice::align_to(size1, ice::ualign::b_1).value == size1);
            CHECK(ice::align_to(size2, ice::ualign::b_1).value == size2);

            CHECK(ice::align_to(size1, ice::ualign::b_1).alignment == ice::ualign::b_1);
            CHECK(ice::align_to(size2, ice::ualign::b_1).alignment == ice::ualign::b_1);
        }

        THEN("aligning values to '2' changes 'size1'...")
        {
            CHECK(ice::align_to(size1, ice::ualign::b_2).padding == 1_B);
            CHECK(ice::align_to(size2, ice::ualign::b_2).padding == 0_B);

            CHECK(ice::align_to(size1, ice::ualign::b_2).value == size1 + 1_B);
            CHECK(ice::align_to(size2, ice::ualign::b_2).value == size2);

            CHECK(ice::align_to(size1, ice::ualign::b_2).alignment == ice::ualign::b_2);
            CHECK(ice::align_to(size2, ice::ualign::b_2).alignment == ice::ualign::b_2);
        }

        THEN("aligning values to '4' changes both values...")
        {
            CHECK(ice::align_to(size1, ice::ualign::b_4).padding == 3_B);
            CHECK(ice::align_to(size2, ice::ualign::b_4).padding == 2_B);

            CHECK(ice::align_to(size1, ice::ualign::b_4).value == size1 + 3_B);
            CHECK(ice::align_to(size2, ice::ualign::b_4).value == size2 + 2_B);

            CHECK(ice::align_to(size1, ice::ualign::b_4).alignment == ice::ualign::b_4);
            CHECK(ice::align_to(size2, ice::ualign::b_4).alignment == ice::ualign::b_4);
        }

        THEN("aligning values to '128' equalizes their size...")
        {
            CHECK(ice::align_to(size1, ice::ualign::b_128).padding == 128_B - size1);
            CHECK(ice::align_to(size2, ice::ualign::b_128).padding == 128_B - size2);

            CHECK(ice::align_to(size1, ice::ualign::b_128).value == 128_B);
            CHECK(
                ice::align_to(size1, ice::ualign::b_128).value
                ==
                ice::align_to(size2, ice::ualign::b_128).value
            );

            CHECK(ice::align_to(size1, ice::ualign::b_128).alignment == ice::ualign::b_128);
            CHECK(ice::align_to(size2, ice::ualign::b_128).alignment == ice::ualign::b_128);
        }
    }
}
