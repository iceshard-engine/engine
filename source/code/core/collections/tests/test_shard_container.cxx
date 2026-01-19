/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/shard.hxx>
#include <ice/shard_container.hxx>

SCENARIO("collections 'ice/shard_container.hxx'", "[shard][collection]")
{
    using ice::operator""_shard;
    using ice::operator""_count;

    static constexpr ice::Shard test_shard_1 = "shard_1"_shard;
    static constexpr ice::Shard test_shard_2 = "shard_2"_shard;

    ice::HostAllocator alloc{ };
    ice::ShardContainer test_container{ alloc };

    GIVEN("an empty shard container")
    {
        REQUIRE(test_container.is_empty());
        REQUIRE(test_container.capacity() == 0_count);

        WHEN("shard 'one' is pushed without a payload")
        {
            test_container.push_back(test_shard_1);

            CHECK(test_container.size() == 1_count);
            CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
            CHECK(test_container.count_of(test_shard_1) == 1);
            CHECK(test_container.count_of(test_shard_2) == 0);

            THEN("shards of type 'one' are removed")
            {
                test_container.remove_all_of(test_shard_1);

                CHECK(test_container.size() == 0_count);
                CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                CHECK(test_container.count_of(test_shard_1) == 0);
                CHECK(test_container.count_of(test_shard_2) == 0);
            }

            THEN("shards of type 'two' are removed")
            {
                test_container.remove_all_of(test_shard_2);

                CHECK(test_container.size() == 1);
                CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                CHECK(test_container.count_of(test_shard_1) == 1);
                CHECK(test_container.count_of(test_shard_2) == 0);
            }

            THEN("inspecting the container returns no results")
            {
                ice::Array<ice::u32> payloads{ alloc };
                ice::ncount const payload_count = test_container.inspect_all(test_shard_1, payloads);

                CHECK(payload_count == 0);
            }
        }

        WHEN("shard 'two' is pushed with a payload")
        {
            constexpr ice::u32 test_payload_value = 0xD00D00;
            test_container.push_back(test_shard_2 | test_payload_value);

            CHECK(test_container.size() == 1);
            CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
            CHECK(test_container.count_of(test_shard_1) == 0);
            CHECK(test_container.count_of(test_shard_2) == 1);

            THEN("shards of type 'one' are removed")
            {
                test_container.remove_all_of(test_shard_1);

                CHECK(test_container.size() == 1);
                CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                CHECK(test_container.count_of(test_shard_1) == 0);
                CHECK(test_container.count_of(test_shard_2) == 1);
            }

            THEN("shards of type 'two' are removed")
            {
                test_container.remove_all_of(test_shard_2);

                CHECK(test_container.size() == 0);
                CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                CHECK(test_container.count_of(test_shard_1) == 0);
                CHECK(test_container.count_of(test_shard_2) == 0);
            }

            THEN("inspecting the container returns one result")
            {
                ice::Array<ice::u32> payloads{ alloc };
                ice::ncount const payload_count = test_container.inspect_all(test_shard_2, payloads);

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

            test_container.push_back(test_shard_1 | test_i32_payload_value1);
            test_container.push_back(test_shard_1 | test_u32_payload_value1);
            test_container.push_back(test_shard_2 | test_i64_payload_value1);
            test_container.push_back(test_shard_3 | test_i32_payload_value2);
            test_container.push_back(test_shard_3 | test_u32_payload_value2);
            test_container.push_back(test_shard_4 | test_i64_payload_value2);


            CHECK(test_container.size() == 6);
            CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
            CHECK(test_container.count_of(test_shard_1) == 2);
            CHECK(test_container.count_of(test_shard_2) == 1);
            CHECK(test_container.count_of(test_shard_3) == 2);
            CHECK(test_container.count_of(test_shard_4) == 1);

            THEN("we add even more shards (without shard+payload duplicates)")
            {
                test_container.push_back(test_shard_1 | test_i64_payload_value1);
                test_container.push_back(test_shard_2 | test_i32_payload_value2);
                test_container.push_back(test_shard_3 | test_i64_payload_value2);
                test_container.push_back(test_shard_4 | test_u32_payload_value1);

                CHECK(test_container.size() == 10);
                CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                CHECK(test_container.count_of(test_shard_1) == 3);
                CHECK(test_container.count_of(test_shard_2) == 2);
                CHECK(test_container.count_of(test_shard_3) == 3);
                CHECK(test_container.count_of(test_shard_4) == 2);

                THEN("inspect using shards without payload-id's")
                {
                    ice::i32 payload_i32 = 1;
                    ice::u32 payload_u32 = 1;
                    ice::i64 payload_i64 = 1;

                    bool inspect_1 = test_container.inspect_first(test_shard_1, payload_i32);
                    bool inspect_2 = test_container.inspect_first(test_shard_2, payload_u32);
                    bool inspect_3 = test_container.inspect_first(test_shard_3, payload_i64);

                    CHECK(inspect_1 == true);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                    CHECK(payload_i32 == test_i32_payload_value1);
                    CHECK(payload_u32 == 1);
                    CHECK(payload_i64 == 1);

                    inspect_1 = test_container.inspect_last(test_shard_4, payload_i32);
                    inspect_2 = test_container.inspect_last(test_shard_3, payload_u32);
                    inspect_3 = test_container.inspect_last(test_shard_2, payload_i64);

                    CHECK(inspect_1 == false);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                }

                THEN("inspect using shards with payload-id's")
                {
                    ice::i32 payload_i32 = 1;
                    ice::u32 payload_u32 = 1;
                    ice::i64 payload_i64 = 1;

                    bool inspect_1 = test_container.inspect_first(test_shard_1 | ice::i32{}, payload_i32);
                    bool inspect_2 = test_container.inspect_first(test_shard_2 | ice::u32{}, payload_u32);
                    bool inspect_3 = test_container.inspect_first(test_shard_3 | ice::u64{}, payload_i64);

                    CHECK(inspect_1 == true);
                    CHECK(inspect_2 == false);
                    CHECK(inspect_3 == false);
                    CHECK(payload_i32 == test_i32_payload_value1);
                    CHECK(payload_u32 == 1);
                    CHECK(payload_i64 == 1);

                    inspect_1 = test_container.inspect_last(test_shard_4 | ice::i32{}, payload_i32);
                    inspect_2 = test_container.inspect_last(test_shard_3 | ice::u32{}, payload_u32);
                    inspect_3 = test_container.inspect_last(test_shard_2 | ice::u64{}, payload_i64);

                    CHECK(inspect_1 == false);
                    CHECK(inspect_2 == true);
                    CHECK(inspect_3 == false);

                    CHECK(payload_u32 == test_u32_payload_value2);
                }

                THEN("we can remove all shards of a specific type (without payload-id)")
                {
                    test_container.remove_all_of(test_shard_1);

                    CHECK(test_container.size() == 7);
                    CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                    CHECK(test_container.count_of(test_shard_1) == 0);
                    CHECK(test_container.count_of(test_shard_2) == 2);
                    CHECK(test_container.count_of(test_shard_3) == 3);
                    CHECK(test_container.count_of(test_shard_4) == 2);

                    CHECK(test_container.contains(test_shard_1) == false);
                    CHECK(test_container.contains(test_shard_2) == true);
                    CHECK(test_container.contains(test_shard_3) == true);
                    CHECK(test_container.contains(test_shard_4) == true);
                }

                THEN("we can remove shards of a specific type (without a payload-id)")
                {
                    test_container.remove_all_of(test_shard_1 | ice::u32{});
                    test_container.remove_all_of(test_shard_2 | ice::u32{});
                    test_container.remove_all_of(test_shard_3 | ice::u32{});
                    test_container.remove_all_of(test_shard_4 | ice::u32{});

                    CHECK(test_container.size() == 7);
                    CHECK(test_container.count_of(ice::Shard_Invalid) == 0);
                    CHECK(test_container.count_of(test_shard_1) == 2);
                    CHECK(test_container.count_of(test_shard_2) == 2);
                    CHECK(test_container.count_of(test_shard_3) == 2);
                    CHECK(test_container.count_of(test_shard_4) == 1);

                    CHECK(test_container.contains(test_shard_1) == true);
                    CHECK(test_container.contains(test_shard_2) == true);
                    CHECK(test_container.contains(test_shard_3) == true);
                    CHECK(test_container.contains(test_shard_4) == true);

                    CHECK(test_container.contains(test_shard_1 | ice::u32{}) == false);
                    CHECK(test_container.contains(test_shard_2 | ice::u32{}) == false);
                    CHECK(test_container.contains(test_shard_3 | ice::u32{}) == false);
                    CHECK(test_container.contains(test_shard_4 | ice::u32{}) == false);
                }
            }

            THEN("we add even more shards (with shard+payload duplicates)")
            {
                ice::ncount const pre_push_size = test_container.size();

                test_container.push_back(test_shard_1 | test_i64_payload_value2);
                test_container.push_back(test_shard_1 | test_i32_payload_value2);
                test_container.push_back(test_shard_1 | test_u32_payload_value2);
                test_container.push_back(test_shard_2 | test_i32_payload_value1);
                test_container.push_back(test_shard_2 | test_i64_payload_value2);
                test_container.push_back(test_shard_3 | test_i32_payload_value1);
                test_container.push_back(test_shard_3 | test_u32_payload_value1);
                test_container.push_back(test_shard_4 | test_u32_payload_value2);
                test_container.push_back(test_shard_4 | test_i64_payload_value1);

                CHECK(test_container.count_of(test_shard_1) == 5);
                CHECK(test_container.count_of(test_shard_2) == 3);
                CHECK(test_container.count_of(test_shard_3) == 4);
                CHECK(test_container.count_of(test_shard_4) == 3);

                THEN("we can inspect multiple payloads")
                {
                    CHECK(test_container.count_of(test_shard_1 | ice::u32{ }) == 2);
                    CHECK(test_container.count_of(test_shard_2 | ice::u32{ }) == 0);
                    CHECK(test_container.count_of(test_shard_3 | ice::u32{ }) == 2);
                    CHECK(test_container.count_of(test_shard_4 | ice::u32{ }) == 1);

                    ice::Array<ice::u32> payloads{ alloc };
                    test_container.inspect_all(test_shard_1, payloads);

                    REQUIRE(payloads.size() == 2);
                    CHECK(payloads[0] == test_u32_payload_value1);
                    CHECK(payloads[1] == test_u32_payload_value2);

                    THEN("we check if we can find these payloads")
                    {
                        ice::u32 payload = 0;
                        ice::Shard first_shard = test_container.find_first_of(test_shard_1 | ice::u32{ }, 0);
                        ice::Shard last_shard = test_container.find_first_of(test_shard_1 | ice::u32{ }, pre_push_size.u32());

                        ice::shard_inspect(first_shard, payload);
                        CHECK(payload == payloads[0]);

                        ice::shard_inspect(last_shard, payload);
                        CHECK(payload == payloads[1]);

                        first_shard = test_container.find_last_of(test_shard_1 | ice::u32{ }, 0);
                        last_shard = test_container.find_last_of(test_shard_1 | ice::u32{ }, (test_container.size() - pre_push_size).u32());

                        ice::shard_inspect(first_shard, payload);
                        CHECK(payload == payloads[1]);

                        ice::shard_inspect(last_shard, payload);
                        CHECK(payload == payloads[0]);
                    }

                    payloads.clear();
                    test_container.inspect_all(test_shard_2, payloads);

                    REQUIRE(payloads.size() == 0);

                    payloads.clear();
                    test_container.inspect_all(test_shard_3, payloads);

                    REQUIRE(payloads.size() == 2);
                    CHECK(payloads[0] == test_u32_payload_value2);
                    CHECK(payloads[1] == test_u32_payload_value1);

                    payloads.clear();
                    test_container.inspect_all(test_shard_4, payloads);

                    REQUIRE(payloads.size() == 1);
                    CHECK(payloads[0] == test_u32_payload_value2);
                }
            }
        }
    }
}
