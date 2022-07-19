#include <catch2/catch.hpp>
#include <ice/mem_unique_ptr.hxx>
#include <ice/mem_allocator_host.hxx>
#include "test_utils.hxx"

struct UniquePtr_TestPOD
{
    ice::i32 x;
};

class UniquePtr_TestObject
{
public:
    UniquePtr_TestObject(ice::i32& counter) noexcept
        : _counter{ counter }
    {
        _counter += 1;
    }

    ~UniquePtr_TestObject() noexcept
    {
        _counter -= 1;
    }

    auto current_counter() const noexcept -> ice::i32 { return _counter; }

private:
    ice::i32& _counter;
};

SCENARIO("memsys 'ice/mem_unique_ptr.hxx'", "[unique_ptr]")
{
    using namespace ice;

    ice::HostAllocator host_alloc{ };

    GIVEN("a host allocator...")
    {
        THEN("we can create a UniquePtr of 'Test_POD'...")
        {
            ice::UniquePtr<UniquePtr_TestPOD> test_pod = ice::make_unique<UniquePtr_TestPOD>(host_alloc, 55);
            CHECK(test_pod->x == 55);

            AND_THEN("we can reassing it another value")
            {
                test_pod = ice::make_unique<UniquePtr_TestPOD>(host_alloc, 33);
                CHECK(test_pod->x == 33);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_alloc.allocation_count() == 1);
                    CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestPOD>);
                }
            }

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_alloc.allocation_count() == 1);
                CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestPOD>);
            }

            WHEN("reseting it removes the object...")
            {
                test_pod.reset();

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_alloc.allocation_count() == 0);
                    CHECK(host_alloc.total_allocated() == 0_B);
                }

                THEN("we can assing it a new_value")
                {
                    test_pod = ice::make_unique<UniquePtr_TestPOD>(host_alloc, 22);
                    CHECK(test_pod->x == 22);
                }
            }
        }
    }

    ice::i32 unique_counter = 0;

    THEN("we can create multiple UniquePtrs of 'Test_Object'...")
    {
        ice::UniquePtr<UniquePtr_TestObject> pointers[3];

        for (auto const& ptr : pointers)
        {
            CHECK(ptr == nullptr);
        }

        pointers[0] = ice::make_unique<UniquePtr_TestObject>(host_alloc, unique_counter);
        pointers[1] = ice::make_unique<UniquePtr_TestObject>(host_alloc, unique_counter);
        pointers[2] = ice::make_unique<UniquePtr_TestObject>(host_alloc, unique_counter);

        CHECK(unique_counter == 3);

        if constexpr (ice::HostAllocator::HasDebugInformation)
        {
            CHECK(host_alloc.allocation_count() == 3);
            CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestObject> * 3);
        }

        WHEN("we move pointers[2] to pointers[1]...")
        {
            pointers[1] = ice::move(pointers[2]);

            THEN("pointer[1] gets destroyed")
            {
                CHECK(unique_counter == 2);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_alloc.allocation_count() == 2);
                    CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestObject> * 2);
                }

                AND_THEN("pointers[2] is empty")
                {
                    CHECK(pointers[2] == nullptr);
                }
            }
        }

        WHEN("we reset pointers[0] it gets destroyed")
        {
            pointers[0].reset();

            CHECK(unique_counter == 2);

            if constexpr (ice::HostAllocator::HasDebugInformation)
            {
                CHECK(host_alloc.allocation_count() == 2);
                CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestObject> * 2);
            }

            AND_THEN("pointers[0] is empty")
            {
                CHECK(pointers[0] == nullptr);
            }

            AND_WHEN("we reset a second time nothing happens")
            {
                pointers[0].reset();

                CHECK(pointers[0] == nullptr);
                CHECK(unique_counter == 2);

                if constexpr (ice::HostAllocator::HasDebugInformation)
                {
                    CHECK(host_alloc.allocation_count() == 2);
                    CHECK(host_alloc.total_allocated() == ice::size_of<UniquePtr_TestObject> * 2);
                }
            }
        }
    }

    CHECK(unique_counter == 0);

    if constexpr (ice::HostAllocator::HasDebugInformation)
    {
        CHECK(host_alloc.allocation_count() == 0);
        CHECK(host_alloc.total_allocated() == 0_B);
    }
}
