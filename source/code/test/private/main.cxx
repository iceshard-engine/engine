#include <core/memory.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/pod/array.hxx>

#include <core/cexpr/stringid.hxx>
#include <core/scope_exit.hxx>

#include <fmt/format.h>

int main()
{
    {
        using namespace core::pod::array;

        core::memory::globals::init();
        auto& a = core::memory::globals::default_allocator();

        constexpr auto sid2 = core::cexpr::stringid_cexpr("test2");
        auto sid = core::cexpr::stringid("test1");

        {
            auto p = a.allocate(10);

            core::scope_guard on_exit_1 = [&]
            {
                a.deallocate(p);
            };

            core::scope_guard on_exit_2 = [&]
            {
                a.deallocate(nullptr);
            };
        }

        fmt::print("{}\n", sid2);
        fmt::print("{}\n", sid);

        core::memory::globals::shutdown();
    }
    return 0;
}
