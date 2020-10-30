#include <catch2/catch.hpp>
#include <ice/memory/memory_globals.hxx>
#include <ice/pod/hash.hxx>

SCENARIO("ice :: pod :: Hash")
{
    namespace hash = ice::pod::hash;
    namespace multi_hash = ice::pod::multi_hash;

    ice::Allocator& alloc = ice::memory::default_allocator();
    ice::pod::Hash<int32_t> test_hash{ alloc };

    GIVEN("An empty hash")
    {
        WHEN("Setting a single value")
        {
            hash::set(test_hash, 0, 0xd00b);
            CHECK(hash::has(test_hash, 0) == true);
            CHECK(hash::get(test_hash, 0, 0xffff) == 0xd00b);
            CHECK(hash::get(test_hash, 1, 0xffff) == 0xffff);
        }

        WHEN("Setting multiple values")
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
                int count = 0;
                for (auto& entry : test_hash)
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
            CHECK(multi_hash::find_first(test_hash, 0)->value == 0xd00b);
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

            CHECK(multi_hash::find_first(test_hash, 0)->value == 0xd00b + 5);
            multi_hash::insert(test_hash, 0, 0xd00b + 0);

            CHECK(multi_hash::count(test_hash, 0) == 3);

            THEN("We got values to iterate over")
            {
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
