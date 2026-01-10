/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/shard.hxx>
#include <ice/shard_container.hxx>

SCENARIO("collections 'ice/shard_container.hxx'", "[shard][collection]")
{
    using ice::operator""_shard;

    static constexpr ice::Shard test_shard_1 = "shard_1"_shard;
    static constexpr ice::Shard test_shard_2 = "shard_2"_shard;

    ice::HostAllocator alloc{ };
    ice::ShardContainer test_container{ alloc };

    GIVEN("an empty shard container")
    {
        REQUIRE(ice::shards::empty(test_container));
        REQUIRE(ice::shards::capacity(test_container) == 0);

        WHEN("shard 'one' is pushed without a payload")
        {
            ice::shards::push_back(test_container, test_shard_1);

            CHECK(ice::shards::size(test_container) == 1);
            CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
            CHECK(ice::shards::count(test_container, test_shard_1) == 1);
            CHECK(ice::shards::count(test_container, test_shard_2) == 0);

            THEN("shards of type 'one' are removed")
            {
                ice::shards::remove_all_of(test_container, test_shard_1);

                CHECK(ice::shards::size(test_container) == 0);
                CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                CHECK(ice::shards::count(test_container, test_shard_1) == 0);
                CHECK(ice::shards::count(test_container, test_shard_2) == 0);
            }

            THEN("shards of type 'two' are removed")
            {
                ice::shards::remove_all_of(test_container, test_shard_2);

                CHECK(ice::shards::size(test_container) == 1);
                CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                CHECK(ice::shards::count(test_container, test_shard_1) == 1);
                CHECK(ice::shards::count(test_container, test_shard_2) == 0);
            }

            THEN("inspecting the container returns no results")
            {
                ice::Array<ice::u32> payloads{ alloc };
                ice::u32 const payload_count = ice::shards::inspect_all(test_container, test_shard_1, payloads);

                CHECK(payload_count == 0);
            }
        }

        WHEN("shard 'two' is pushed with a payload")
        {
            constexpr ice::u32 test_payload_value = 0xD00D00;
            ice::shards::push_back(test_container, test_shard_2 | test_payload_value);

            CHECK(ice::shards::size(test_container) == 1);
            CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
            CHECK(ice::shards::count(test_container, test_shard_1) == 0);
            CHECK(ice::shards::count(test_container, test_shard_2) == 1);

            THEN("shards of type 'one' are removed")
            {
                ice::shards::remove_all_of(test_container, test_shard_1);

                CHECK(ice::shards::size(test_container) == 1);
                CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                CHECK(ice::shards::count(test_container, test_shard_1) == 0);
                CHECK(ice::shards::count(test_container, test_shard_2) == 1);
            }

            THEN("shards of type 'two' are removed")
            {
                ice::shards::remove_all_of(test_container, test_shard_2);

                CHECK(ice::shards::size(test_container) == 0);
                CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                CHECK(ice::shards::count(test_container, test_shard_1) == 0);
                CHECK(ice::shards::count(test_container, test_shard_2) == 0);
            }

            THEN("inspecting the container returns one result")
            {
                ice::Array<ice::u32> payloads{ alloc };
                ice::u32 const payload_count = ice::shards::inspect_all(test_container, test_shard_2, payloads);

                REQUIRE(payload_count == 1);
                CHECK(payloads[0] == test_payload_value);
            }
        }

        WHEN("multiple shards are pushed with payloads")
        {
            constexpr ice::i32 test_i32_payload_value1 = 0x103232;
            constexpr ice::i32 test_i32_payload_value2 = -0x103232;
            constexpr ice::u32 test_u32_payload_value1 = 0x103232;
            constexpr ice::u32 test_u32_payload_value2 = ~0x103232u;
            constexpr ice::i64 test_i64_payload_value1 = 0x7f000000'000000ff;
            constexpr ice::i64 test_i64_payload_value2 = 0xff000000'000000ff;

            static constexpr ice::Shard test_shard_3 = "shard_3"_shard;
            static constexpr ice::Shard test_shard_4 = "shard_4"_shard;

            ice::shards::push_back(test_container, test_shard_1 | test_i32_payload_value1);
            ice::shards::push_back(test_container, test_shard_1 | test_u32_payload_value1);
            ice::shards::push_back(test_container, test_shard_2 | test_i64_payload_value1);
            ice::shards::push_back(test_container, test_shard_3 | test_i32_payload_value2);
            ice::shards::push_back(test_container, test_shard_3 | test_u32_payload_value2);
            ice::shards::push_back(test_container, test_shard_4 | test_i64_payload_value2);


            CHECK(ice::shards::size(test_container) == 6);
            CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
            CHECK(ice::shards::count(test_container, test_shard_1) == 2);
            CHECK(ice::shards::count(test_container, test_shard_2) == 1);
            CHECK(ice::shards::count(test_container, test_shard_3) == 2);
            CHECK(ice::shards::count(test_container, test_shard_4) == 1);

            THEN("we add even more shards (without shard+payload duplicates)")
            {
                ice::shards::push_back(test_container, test_shard_1 | test_i64_payload_value1);
                ice::shards::push_back(test_container, test_shard_2 | test_i32_payload_value2);
                ice::shards::push_back(test_container, test_shard_3 | test_i64_payload_value2);
                ice::shards::push_back(test_container, test_shard_4 | test_u32_payload_value1);

                CHECK(ice::shards::size(test_container) == 10);
                CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                CHECK(ice::shards::count(test_container, test_shard_1) == 3);
                CHECK(ice::shards::count(test_container, test_shard_2) == 2);
                CHECK(ice::shards::count(test_container, test_shard_3) == 3);
                CHECK(ice::shards::count(test_container, test_shard_4) == 2);

                THEN("inspect using shards without payload-id's")
                {
                    ice::i32 payload_i32 = 1;
                    ice::u32 payload_u32 = 1;
                    ice::i64 payload_i64 = 1;

                    bool inspect_1 = ice::shards::inspect_first(test_container, test_shard_1, payload_i32);
                    bool inspect_2 = ice::shards::inspect_first(test_container, test_shard_2, payload_u32);
                    bool inspect_3 = ice::shards::inspect_first(test_container, test_shard_3, payload_i64);

                    CHECK(inspect_1 == true);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                    CHECK(payload_i32 == test_i32_payload_value1);
                    CHECK(payload_u32 == 1);
                    CHECK(payload_i64 == 1);

                    inspect_1 = ice::shards::inspect_last(test_container, test_shard_4, payload_i32);
                    inspect_2 = ice::shards::inspect_last(test_container, test_shard_3, payload_u32);
                    inspect_3 = ice::shards::inspect_last(test_container, test_shard_2, payload_i64);

                    CHECK(inspect_1 == false);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                }

                THEN("inspect using shards with payload-id's")
                {
                    ice::i32 payload_i32 = 1;
                    ice::u32 payload_u32 = 1;
                    ice::i64 payload_i64 = 1;

                    bool inspect_1 = ice::shards::inspect_first(test_container, test_shard_1 | ice::i32{}, payload_i32);
                    bool inspect_2 = ice::shards::inspect_first(test_container, test_shard_2 | ice::u32{}, payload_u32);
                    bool inspect_3 = ice::shards::inspect_first(test_container, test_shard_3 | ice::u64{}, payload_i64);

                    CHECK(inspect_1 == true);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                    CHECK(payload_i32 == test_i32_payload_value1);
                    CHECK(payload_u32 == 1);
                    CHECK(payload_i64 == 1);

                    inspect_1 = ice::shards::inspect_last(test_container, test_shard_4 | ice::i32{}, payload_i32);
                    inspect_2 = ice::shards::inspect_last(test_container, test_shard_3 | ice::u32{}, payload_u32);
                    inspect_3 = ice::shards::inspect_last(test_container, test_shard_2 | ice::u64{}, payload_i64);

                    CHECK(inspect_1 == false);
                    CHECK(inspect_2 == true);
                    CHECK(inspect_3 == false);

                    CHECK(payload_u32 == test_u32_payload_value2);
                }

                THEN("we can remove all shards of a specific type (without payload-id)")
                {
                    ice::shards::remove_all_of(test_container, test_shard_1);

                    CHECK(ice::shards::size(test_container) == 7);
                    CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                    CHECK(ice::shards::count(test_container, test_shard_1) == 0);
                    CHECK(ice::shards::count(test_container, test_shard_2) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_3) == 3);
                    CHECK(ice::shards::count(test_container, test_shard_4) == 2);

                    CHECK(ice::shards::contains(test_container, test_shard_1) == false);
                    CHECK(ice::shards::contains(test_container, test_shard_2) == true);
                    CHECK(ice::shards::contains(test_container, test_shard_3) == true);
                    CHECK(ice::shards::contains(test_container, test_shard_4) == true);
                }

                THEN("we can remove shards of a specific type (without a payload-id)")
                {
                    ice::shards::remove_all_of(test_container, test_shard_1 | ice::u32{});
                    ice::shards::remove_all_of(test_container, test_shard_2 | ice::u32{});
                    ice::shards::remove_all_of(test_container, test_shard_3 | ice::u32{});
                    ice::shards::remove_all_of(test_container, test_shard_4 | ice::u32{});

                    CHECK(ice::shards::size(test_container) == 7);
                    CHECK(ice::shards::count(test_container, ice::Shard_Invalid) == 0);
                    CHECK(ice::shards::count(test_container, test_shard_1) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_2) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_3) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_4) == 1);

                    CHECK(ice::shards::contains(test_container, test_shard_1) == true);
                    CHECK(ice::shards::contains(test_container, test_shard_2) == true);
                    CHECK(ice::shards::contains(test_container, test_shard_3) == true);
                    CHECK(ice::shards::contains(test_container, test_shard_4) == true);

                    CHECK(ice::shards::contains(test_container, test_shard_1 | ice::u32{}) == false);
                    CHECK(ice::shards::contains(test_container, test_shard_2 | ice::u32{}) == false);
                    CHECK(ice::shards::contains(test_container, test_shard_3 | ice::u32{}) == false);
                    CHECK(ice::shards::contains(test_container, test_shard_4 | ice::u32{}) == false);
                }
            }

            THEN("we add even more shards (with shard+payload duplicates)")
            {
                ice::u32 const pre_push_size = ice::shards::size(test_container);

                ice::shards::push_back(test_container, test_shard_1 | test_i64_payload_value2);
                ice::shards::push_back(test_container, test_shard_1 | test_i32_payload_value2);
                ice::shards::push_back(test_container, test_shard_1 | test_u32_payload_value2);
                ice::shards::push_back(test_container, test_shard_2 | test_i32_payload_value1);
                ice::shards::push_back(test_container, test_shard_2 | test_i64_payload_value2);
                ice::shards::push_back(test_container, test_shard_3 | test_i32_payload_value1);
                ice::shards::push_back(test_container, test_shard_3 | test_u32_payload_value1);
                ice::shards::push_back(test_container, test_shard_4 | test_u32_payload_value2);
                ice::shards::push_back(test_container, test_shard_4 | test_i64_payload_value1);

                CHECK(ice::shards::count(test_container, test_shard_1) == 5);
                CHECK(ice::shards::count(test_container, test_shard_2) == 3);
                CHECK(ice::shards::count(test_container, test_shard_3) == 4);
                CHECK(ice::shards::count(test_container, test_shard_4) == 3);

                THEN("we can inspect multiple payloads")
                {
                    CHECK(ice::shards::count(test_container, test_shard_1 | ice::u32{ }) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_2 | ice::u32{ }) == 0);
                    CHECK(ice::shards::count(test_container, test_shard_3 | ice::u32{ }) == 2);
                    CHECK(ice::shards::count(test_container, test_shard_4 | ice::u32{ }) == 1);

                    ice::Array<ice::u32> payloads{ alloc };
                    ice::shards::inspect_all(test_container, test_shard_1, payloads);

                    REQUIRE(payloads.size() == 2);
                    CHECK(payloads[0] == test_u32_payload_value1);
                    CHECK(payloads[1] == test_u32_payload_value2);

                    THEN("we check if we can find these payloads")
                    {
                        ice::u32 payload = 0;
                        ice::Shard first_shard = ice::shards::find_first_of(test_container, test_shard_1 | ice::u32{ }, 0);
                        ice::Shard last_shard = ice::shards::find_first_of(test_container, test_shard_1 | ice::u32{ }, pre_push_size);

                        ice::shard_inspect(first_shard, payload);
                        CHECK(payload == payloads[0]);

                        ice::shard_inspect(last_shard, payload);
                        CHECK(payload == payloads[1]);

                        first_shard = ice::shards::find_last_of(test_container, test_shard_1 | ice::u32{ }, 0);
                        last_shard = ice::shards::find_last_of(test_container, test_shard_1 | ice::u32{ }, ice::shards::size(test_container) - pre_push_size);

                        ice::shard_inspect(first_shard, payload);
                        CHECK(payload == payloads[1]);

                        ice::shard_inspect(last_shard, payload);
                        CHECK(payload == payloads[0]);
                    }

                    ice::array::clear(payloads);
                    ice::shards::inspect_all(test_container, test_shard_2, payloads);

                    REQUIRE(payloads.size() == 0);

                    ice::array::clear(payloads);
                    ice::shards::inspect_all(test_container, test_shard_3, payloads);

                    REQUIRE(payloads.size() == 2);
                    CHECK(payloads[0] == test_u32_payload_value2);
                    CHECK(payloads[1] == test_u32_payload_value1);

                    ice::array::clear(payloads);
                    ice::shards::inspect_all(test_container, test_shard_4, payloads);

                    REQUIRE(payloads.size() == 1);
                    CHECK(payloads[0] == test_u32_payload_value2);
                }
            }
        }
    }
}
