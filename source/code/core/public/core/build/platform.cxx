#include <core/build/platform.hxx>
#include <cassert>

auto core::build::platform::to_string(System type) noexcept -> const char*
{
    switch (type)
    {
    case core::build::platform::System::Windows:
        return "windows";
    default:
        break;
    }
    assert(false);
    return nullptr;
}

auto core::build::platform::to_string(Architecture type) noexcept -> const char*
{
    switch (type)
    {
    case core::build::platform::Architecture::x64:
        return "x86_64";
    default:
        break;
    }
    assert(false);
    return nullptr;
}

auto core::build::platform::to_string(Compiler type) noexcept -> const char*
{
    switch (type)
    {
    case core::build::platform::Compiler::MSVC:
        return "msvc";
    default:
        break;
    }
    assert(false);
    return nullptr;
}

auto core::build::platform::to_string(Platform type) noexcept -> const char*
{
    return type.name;
}
