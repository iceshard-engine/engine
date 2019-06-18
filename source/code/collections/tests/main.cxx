#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <core/memory.hxx>

int main(int argc, char* argv[])
{
    int result = 0;

    {
        core::memory::globals::init();

        result = Catch::Session().run(argc, argv);

        core::memory::globals::shutdown();
    }

    return result;
}