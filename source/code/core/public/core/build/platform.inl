#include "platform.hxx"

constexpr bool core::build::platform::operator==(const Platform& left, const Platform& right) noexcept
{
    return left.system == right.system
        && left.architecture == right.architecture
        && left.compiler == right.compiler;
}

constexpr bool core::build::platform::operator!=(const Platform& left, const Platform& right) noexcept
{
    return !(left == right);
}

constexpr bool core::build::platform::operator==(const Platform& left, System right) noexcept
{
    return left.system == right;
}

constexpr bool core::build::platform::operator!=(const Platform& left, System right) noexcept
{
    return !(left == right);
}

constexpr bool core::build::platform::operator==(const Platform& left, Architecture right) noexcept
{
    return left.architecture == right;
}

constexpr bool core::build::platform::operator!=(const Platform& left, Architecture right) noexcept
{
    return !(left == right);
}

constexpr bool core::build::platform::operator==(const Platform& left, Compiler right) noexcept
{
    return left.compiler == right;
}

constexpr bool core::build::platform::operator!=(const Platform& left, Compiler right) noexcept
{
    return !(left == right);
}
