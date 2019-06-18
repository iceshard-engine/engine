#include <core/memory.hxx>
#include <fmt/format.h>

int main()
{
    {
        core::memory::globals::init();

        auto& a = core::memory::globals::default_allocator();

        auto initial_alloc = a.total_allocated();

        [[maybe_unused]]
        auto* p = a.allocate(1);

        fmt::print("{}", a.total_allocated() - initial_alloc);

        a.deallocate(p);
        core::memory::globals::shutdown();
    }
    return 0;
}
