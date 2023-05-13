/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/hash/murmur2.hxx>
#include <ice/hash/murmur3.hxx>

uint32_t constexpr test_seed_1 = 0xD00D00;
uint32_t constexpr test_seed_2 = 0xDAADAA;

std::u8string_view constexpr test_string_empty = u8"";
std::u8string_view constexpr test_string_short = u8"Fox";
std::u8string_view constexpr test_string_medium = u8"The quick brown fox.";
std::u8string_view constexpr test_string_large = u8"The quick brown fox jumps over the lazy dog.";

TEST_CASE("core 'ice/hash/murmur2.hxx' (64bit)", "[hash]")
{
    namespace mm2h = ice::detail::murmur2_hash;

    mm2h::mm2_x64_64 hash_result_empty = mm2h::cexpr_murmur2_x64_64(test_string_empty, test_seed_1);
    mm2h::mm2_x64_64 hash_result_short = mm2h::cexpr_murmur2_x64_64(test_string_short, test_seed_1);
    mm2h::mm2_x64_64 hash_result_medium = mm2h::cexpr_murmur2_x64_64(test_string_medium, test_seed_1);
    mm2h::mm2_x64_64 hash_result_large = mm2h::cexpr_murmur2_x64_64(test_string_large, test_seed_1);

    // Values currently pre-calculated
    CHECK(hash_result_empty.h[0] == 0x843d163771ed997a);
    CHECK(hash_result_short.h[0] == 0xd3e001b9db0d2ad6);
    CHECK(hash_result_medium.h[0] == 0x763df63e85f894b6);
    CHECK(hash_result_large.h[0] == 0x6cac60b66c6d43d8);

    SECTION("Different seeds")
    {
        mm2h::mm2_x64_64 hash_result2_empty = mm2h::cexpr_murmur2_x64_64(test_string_empty, test_seed_2);
        mm2h::mm2_x64_64 hash_result2_short = mm2h::cexpr_murmur2_x64_64(test_string_short, test_seed_2);
        mm2h::mm2_x64_64 hash_result2_medium = mm2h::cexpr_murmur2_x64_64(test_string_medium, test_seed_2);
        mm2h::mm2_x64_64 hash_result2_large = mm2h::cexpr_murmur2_x64_64(test_string_large, test_seed_2);

        CHECK(hash_result_empty.h[0] != hash_result2_empty.h[0]);
        CHECK(hash_result_short.h[0] != hash_result2_short.h[0]);
        CHECK(hash_result_medium.h[0] != hash_result2_medium.h[0]);
        CHECK(hash_result_large.h[0] != hash_result2_large.h[0]);
    }
}

TEST_CASE("core 'ice/hash/murmur3.hxx' (32bit)", "[hash][32bit_hash]")
{
    namespace mm3h = ice::detail::murmur3_hash;

    mm3h::mm3_x86_h32 hash_result_empty = mm3h::cexpr_murmur3_x86_32(test_string_empty, test_seed_1);
    mm3h::mm3_x86_h32 hash_result_short = mm3h::cexpr_murmur3_x86_32(test_string_short, test_seed_1);
    mm3h::mm3_x86_h32 hash_result_medium = mm3h::cexpr_murmur3_x86_32(test_string_medium, test_seed_1);
    mm3h::mm3_x86_h32 hash_result_large = mm3h::cexpr_murmur3_x86_32(test_string_large, test_seed_1);

    // Values currently pre-calculated
    CHECK(hash_result_empty.h[0] == 0x7156dc6d);
    CHECK(hash_result_short.h[0] == 0xad1cc75);
    CHECK(hash_result_medium.h[0] == 0x4205bfc4);
    CHECK(hash_result_large.h[0] == 0x27080d2);

    SECTION("Different seeds")
    {
        mm3h::mm3_x86_h32 hash_result2_empty = mm3h::cexpr_murmur3_x86_32(test_string_empty, test_seed_2);
        mm3h::mm3_x86_h32 hash_result2_short = mm3h::cexpr_murmur3_x86_32(test_string_short, test_seed_2);
        mm3h::mm3_x86_h32 hash_result2_medium = mm3h::cexpr_murmur3_x86_32(test_string_medium, test_seed_2);
        mm3h::mm3_x86_h32 hash_result2_large = mm3h::cexpr_murmur3_x86_32(test_string_large, test_seed_2);

        CHECK(hash_result_empty.h[0] != hash_result2_empty.h[0]);
        CHECK(hash_result_short.h[0] != hash_result2_short.h[0]);
        CHECK(hash_result_medium.h[0] != hash_result2_medium.h[0]);
        CHECK(hash_result_large.h[0] != hash_result2_large.h[0]);
    }
}

TEST_CASE("core 'ice/hash/murmur3.hxx' (128bit)", "[hash][128bit_hash]")
{
    namespace mm3h = ice::detail::murmur3_hash;

    mm3h::mm3_x64_h128 hash_result_empty = mm3h::cexpr_murmur3_x64_128(test_string_empty, test_seed_1);
    mm3h::mm3_x64_h128 hash_result_short = mm3h::cexpr_murmur3_x64_128(test_string_short, test_seed_1);
    mm3h::mm3_x64_h128 hash_result_medium = mm3h::cexpr_murmur3_x64_128(test_string_medium, test_seed_1);
    mm3h::mm3_x64_h128 hash_result_large = mm3h::cexpr_murmur3_x64_128(test_string_large, test_seed_1);

    // Values currently pre-calculated
    CHECK(hash_result_empty.h[0] == 0x6f2526a7e7951453);
    CHECK(hash_result_empty.h[1] == 0x4d387346f4ebd74d);
    CHECK(hash_result_short.h[0] == 0x5ac5cf6617010a47);
    CHECK(hash_result_short.h[1] == 0x3f17ffcd1642aafd);
    CHECK(hash_result_medium.h[0] == 0x39940de848029cde);
    CHECK(hash_result_medium.h[1] == 0xd2b8dc1a3ddc6867);
    CHECK(hash_result_large.h[0] == 0x727a445180f4e016);
    CHECK(hash_result_large.h[1] == 0x684d1ce7fa233374);

    SECTION("Different seeds")
    {
        mm3h::mm3_x64_h128 hash_result2_empty = mm3h::cexpr_murmur3_x64_128(test_string_empty, test_seed_2);
        mm3h::mm3_x64_h128 hash_result2_short = mm3h::cexpr_murmur3_x64_128(test_string_short, test_seed_2);
        mm3h::mm3_x64_h128 hash_result2_medium = mm3h::cexpr_murmur3_x64_128(test_string_medium, test_seed_2);
        mm3h::mm3_x64_h128 hash_result2_large = mm3h::cexpr_murmur3_x64_128(test_string_large, test_seed_2);

        CHECK(hash_result_empty.h[0] != hash_result2_empty.h[0]);
        CHECK(hash_result_empty.h[1] != hash_result2_empty.h[1]);
        CHECK(hash_result_short.h[0] != hash_result2_short.h[0]);
        CHECK(hash_result_short.h[1] != hash_result2_short.h[1]);
        CHECK(hash_result_medium.h[0] != hash_result2_medium.h[0]);
        CHECK(hash_result_medium.h[1] != hash_result2_medium.h[1]);
        CHECK(hash_result_large.h[0] != hash_result2_large.h[0]);
        CHECK(hash_result_large.h[1] != hash_result2_large.h[1]);
    }
}
