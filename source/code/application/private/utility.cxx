#include <application/utility.hxx>
#include <core/stack_string.hxx>

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace app
{

    auto location(core::allocator& alloc) noexcept -> core::String<>
    {
        core::StackString<256> buffer = "";
        GetModuleFileName(NULL, core::string::begin(buffer), core::string::capacity(buffer));

        return core::String<>{ alloc, std::filesystem::canonical(core::string::begin(buffer)).parent_path().generic_string() };
    }

    auto working_directory(core::allocator& alloc) noexcept -> core::String<>
    {
        core::StackString<256> buffer = "";
        GetCurrentDirectory(core::string::capacity(buffer), core::string::begin(buffer));

        return core::String<>{ alloc, std::filesystem::canonical(core::string::begin(buffer)).generic_string() };
    }

} // namespace app
