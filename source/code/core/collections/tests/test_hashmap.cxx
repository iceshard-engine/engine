/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <ice/mem_allocator_host.hxx>
#include <ice/container/hashmap.hxx>
#include "util_tracking_object.hxx"

SCENARIO("collections 'ice/container/hashmap.hxx'", "[collection][hash][complex]")
{
    namespace hash = ice::hashmap;
    namespace multi_hash = ice::multi_hashmap;

    ice::HostAllocator alloc{ };
    ice::HashMap<Test_TrackingObject, ice::ContainerLogic::Complex> test_hash{ alloc };

    GIVEN("an hashmap with a single element")
    {
        ice::hashmap::set(test_hash, 0, Test_TrackingObject{ 42 });

        Test_TrackingObject* obj = ice::hashmap::try_get(test_hash, 0);
        REQUIRE(obj != nullptr);

        {
            Test_ObjectEvents test_events{ };
            obj->gather_ctors(test_events);

            CHECK(obj->value == 42);
            CHECK(test_events.test_ctor == 0);
            CHECK(test_events.test_ctor_move == 1);
            CHECK(test_events.test_ctor_copy == 0);
        }

        AND_THEN("replacing the object will call destructor")
        {
            ice::ucount dtor_count = 0;
            obj->data.test_dtor = &dtor_count;

            ice::hashmap::set(test_hash, 0, Test_TrackingObject{ 69 });

            obj = ice::hashmap::try_get(test_hash, 0);
            REQUIRE(obj != nullptr);

            Test_ObjectEvents test_events{ };
            obj->gather_ctors(test_events);

            CHECK(obj->value == 69);
            CHECK(test_events.test_ctor == 0);
            CHECK(test_events.test_ctor_move == 1);
            CHECK(test_events.test_ctor_copy == 0);

            CHECK(dtor_count == 1);
        }
    }

    GIVEN("an hashmap with a multiple elements")
    {
        static constexpr ice::u32 values[]{ 42, 11, 23 };

        for (ice::u32 value : values)
        {
            ice::hashmap::set(test_hash, value, Test_TrackingObject{ value });
        }

        Test_ObjectEvents test_events{};
        for (Test_TrackingObject const& obj : test_hash)
        {
            obj.gather_ctors(test_events);
        }

        CHECK(ice::hashmap::count(test_hash) == 3);
        CHECK(test_events.test_ctor == 0);
        CHECK(test_events.test_ctor_move == 3);
        CHECK(test_events.test_ctor_copy == 0);

        AND_THEN("replacing the objects will call destructors")
        {
            ice::ucount dtor_count = 0;
            for (Test_TrackingObject& obj : ice::hashmap::values(test_hash))
            {
                obj.data.test_dtor = &dtor_count;
            }

            for (ice::u32 value : values)
            {
                ice::hashmap::set(test_hash, value, Test_TrackingObject{ value });
            }

            test_events = Test_ObjectEvents{};
            for (Test_TrackingObject const& obj : test_hash)
            {
                obj.gather_ctors(test_events);
            }

            CHECK(ice::hashmap::count(test_hash) == 3);
            CHECK(test_events.test_ctor == 0);
            CHECK(test_events.test_ctor_move == 3);
            CHECK(test_events.test_ctor_copy == 0);

            CHECK(dtor_count == 3);
        }
    }
}

SCENARIO("collections 'ice/container/hashmap.hxx' (POD)", "[collection][hash][pod]")
{
    namespace hash = ice::hashmap;
    namespace multi_hash = ice::multi_hashmap;

    ice::HostAllocator alloc{ };
    ice::HashMap<ice::i32> test_hash{ alloc };

    ice::hashmap::detail::rehash(test_hash, 2);

    GIVEN("an empty hash container")
    {
        WHEN("setting a single value")
        {
            hash::set(test_hash, 0, 0xd00b);
            CHECK(hash::has(test_hash, 0) == true);
            CHECK(hash::get(test_hash, 0, 0xffff) == 0xd00b);
            CHECK(hash::get(test_hash, 1, 0xffff) == 0xffff);
        }

        WHEN("setting multiple values")
        {
            hash::set(test_hash, 0, 0xd00b + 0);
            hash::set(test_hash, 2, 0xd00b + 1);
            hash::set(test_hash, 4, 0xd00b + 2);
            hash::set(test_hash, 6, 0xd00b + 3);

            CHECK(hash::has(test_hash, 0) == true);
            CHECK(hash::has(test_hash, 2) == true);
            CHECK(hash::has(test_hash, 4) == true);
            CHECK(hash::has(test_hash, 6) == true);

            hash::remove(test_hash, 0);
            hash::remove(test_hash, 4);

            CHECK(hash::has(test_hash, 0) == false);
            CHECK(hash::has(test_hash, 2) == true);
            CHECK(hash::has(test_hash, 4) == false);
            CHECK(hash::has(test_hash, 6) == true);

            hash::set(test_hash, 0, 0xd00b + 4);
            hash::set(test_hash, 0, 0xd00b + 5); // Replaces the old value

            CHECK(hash::has(test_hash, 0) == true);
            CHECK(hash::has(test_hash, 2) == true);
            CHECK(hash::has(test_hash, 4) == false);
            CHECK(hash::has(test_hash, 6) == true);

            CHECK(hash::get(test_hash, 0, 0xffff) == 0xd00b + 5);
            hash::set(test_hash, 0, 0xd00b + 0);

            THEN("We got values to iterate over")
            {
                ice::i32 count = 0;
                for (auto& entry : hash::entries(test_hash))
                {
                    count += (entry, 1);
                }

                CHECK(count == 3);
            }

            THEN("We clear the hash")
            {
                hash::clear(test_hash);

                CHECK(hash::has(test_hash, 0) == false);
                CHECK(hash::has(test_hash, 2) == false);
                CHECK(hash::has(test_hash, 4) == false);
                CHECK(hash::has(test_hash, 6) == false);
            }
        }
    }

    GIVEN("An empty multi hash")
    {
        WHEN("Setting a single value")
        {
            multi_hash::insert(test_hash, 0, 0xd00b);
            CHECK(multi_hash::count(test_hash, 0) == 1);
            CHECK(multi_hash::find_first(test_hash, 0).value() == 0xd00b);
            CHECK(multi_hash::find_first(test_hash, 1) == nullptr);
        }

        WHEN("Setting multiple values")
        {
            multi_hash::insert(test_hash, 0, 0xd00b + 0);
            multi_hash::insert(test_hash, 2, 0xd00b + 1);
            multi_hash::insert(test_hash, 4, 0xd00b + 2);
            multi_hash::insert(test_hash, 6, 0xd00b + 3);

            CHECK(multi_hash::count(test_hash, 0) == 1);
            CHECK(multi_hash::count(test_hash, 2) == 1);
            CHECK(multi_hash::count(test_hash, 4) == 1);
            CHECK(multi_hash::count(test_hash, 6) == 1);

            multi_hash::remove_all(test_hash, 0);
            multi_hash::remove_all(test_hash, 4);

            CHECK(multi_hash::count(test_hash, 0) == 0);
            CHECK(multi_hash::count(test_hash, 2) == 1);
            CHECK(multi_hash::count(test_hash, 4) == 0);
            CHECK(multi_hash::count(test_hash, 6) == 1);

            multi_hash::insert(test_hash, 0, 0xd00b + 4);
            multi_hash::insert(test_hash, 0, 0xd00b + 5); // Is added as the new head

            CHECK(multi_hash::count(test_hash, 0) == 2);
            CHECK(multi_hash::count(test_hash, 2) == 1);
            CHECK(multi_hash::count(test_hash, 4) == 0);
            CHECK(multi_hash::count(test_hash, 6) == 1);

            CHECK(multi_hash::find_first(test_hash, 0).value() == 0xd00b + 5);
            multi_hash::insert(test_hash, 0, 0xd00b + 0);

            CHECK(multi_hash::count(test_hash, 0) == 3);

            THEN("We got values to iterate over")
            {
                auto it = ice::begin(test_hash);
                auto const end = ice::end(test_hash);

                int count = 0;
                for (auto& entry : test_hash)
                {
                    count += (entry, 1);
                }

                CHECK(count == 5);
            }

            THEN("We clear the hash")
            {
                hash::clear(test_hash);

                CHECK(multi_hash::count(test_hash, 0) == 0);
                CHECK(multi_hash::count(test_hash, 2) == 0);
                CHECK(multi_hash::count(test_hash, 4) == 0);
                CHECK(multi_hash::count(test_hash, 6) == 0);
            }
        }
    }
}
