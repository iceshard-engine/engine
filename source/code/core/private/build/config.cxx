#include <core/build/config.hxx>
#include <core/debug/assert.hxx>

namespace core::build::configuration
{

auto to_string(ConfigurationType type) noexcept -> const char*
{
    switch (type)
    {
    case core::build::configuration::ConfigurationType::Debug:
        return "Debug";
    case core::build::configuration::ConfigurationType::ReleaseDebug:
        return "ReleaseDebug";
    case core::build::configuration::ConfigurationType::Release:
        return "Release";
    default:
        break;
    }
    IS_ASSERT(false, "ConfigurationType value was not recognized! [ value:{} ]", static_cast<std::underlying_type_t<ConfigurationType>>(type));
    return nullptr;
}

auto to_string(Configuration type) noexcept -> const char*
{
    return type.name;
}

} // namespace core::build::configuration
