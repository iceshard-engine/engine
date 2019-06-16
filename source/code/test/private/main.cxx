#include <memsys/memsys.hxx>
#include <fmt/format.h>

int main()
{
    {
        memsys::globals::init();

        auto& a = memsys::globals::default_allocator();

        auto initial_alloc = a.total_allocated();

        [[maybe_unused]]
        auto* p = a.allocate(1);

        fmt::print("{}", a.total_allocated() - initial_alloc);

        a.deallocate(p);
        memsys::globals::shutdown();
    }
    return 0;
}
