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
            core::StackString sstr = "asd";

            core::String str{ a, "Test12345" };

            str += str;

            fmt::print("{}", str);
        }

        core::memory::globals::shutdown();
    }
    return 0;
}
