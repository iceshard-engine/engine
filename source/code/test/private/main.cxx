#include <core/memory.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <fmt/format.h>

int main()
{
    {
        core::memory::globals::init();
        auto& a = core::memory::globals::default_allocator();

        {
            core::StackString<64> sstr = "Hello";
            core::String str{ a, " World" };

            str += sstr;
            sstr += str;

            fmt::print("{}\n", str);
            fmt::print("{}\n", sstr);
        }

        core::memory::globals::shutdown();
    }
    return 0;
}
