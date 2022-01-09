#include <ice/app_info.hxx>
#include <ice/heap_string.hxx>
#include <ice/stack_string.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>

#include <filesystem>

namespace ice
{

#if ISP_WINDOWS

    void app_location(ice::HeapString<>& out) noexcept
    {
        ice::StackString<256> temp{ "" };
        GetModuleFileNameA(NULL, ice::string::begin(temp), ice::string::capacity(temp));
        out = std::filesystem::canonical(ice::string::data(temp)).parent_path().generic_string();
    }

    void working_directory(ice::HeapString<>& out) noexcept
    {
        ice::StackString<256> temp{ "" };
        GetCurrentDirectoryA(ice::string::capacity(temp), ice::string::begin(temp));
        out = std::filesystem::canonical(ice::string::data(temp)).generic_string();
    }

#else

    void location(ice::HeapString<>& out) noexcept
    {
        ice::string::clear(out);
    }

    void working_directory(ice::HeapString<>& out) noexcept
    {
        ice::string::clear(out);
    }

#endif

} // namespace core
