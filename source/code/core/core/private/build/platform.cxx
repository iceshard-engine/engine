#include <core/build/platform.hxx>
#include <core/debug/assert.hxx>

namespace core::build::platform
{

    auto to_string(Platform type) noexcept -> const char*
    {
        return type.name;
    }

    auto to_string(System type) noexcept -> const char*
    {
        switch (type)
        {
        case core::build::platform::System::Windows:
            return "windows";
        default:
            break;
        }
        IS_FAIL("System value was not recognized! [ value:{} ]", static_cast<std::underlying_type_t<System>>(type));
        return nullptr;
    }

    auto to_string(Architecture type) noexcept -> const char*
    {
        switch (type)
        {
        case core::build::platform::Architecture::x64:
            return "x64";
        default:
            break;
        }
        IS_FAIL("Architecture value was not recognized! [ value:{} ]", static_cast<std::underlying_type_t<Architecture>>(type));
        return nullptr;
    }

    auto to_string(Compiler type) noexcept -> const char*
    {
        switch (type)
        {
        case core::build::platform::Compiler::MSVC:
            return "msvc";
        default:
            break;
        }
        IS_ASSERT(false, "Unknown compiler family");
        return nullptr;
    }

} // namespace core::build::platform
