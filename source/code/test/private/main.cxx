#include <core/memory.hxx>
#include <core/string.hxx>
#include <core/stack_string.hxx>
#include <core/pod/array.hxx>

#include <fmt/format.h>

int main()
{
    {
        using namespace core::pod::array;

        core::memory::globals::init();
        auto& a = core::memory::globals::default_allocator();

        {
            core::pod::Array<int> arr{ a };
            push_back(arr, 33);

            core::StackString<64> sstr = "Hello";
            core::String str{ a, " World" };

            str += sstr;
            sstr += str;

            fmt::print("{}\n", str);
            fmt::print("{}\n", sstr);

            set_capacity(arr, 0);
        }

        core::memory::globals::shutdown();
    }
    return 0;
}
