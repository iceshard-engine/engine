
constexpr bool core::build::configuration::operator==(const Configuration& left, const Configuration& right) noexcept
{
    return left.type == right.type;
}

constexpr bool core::build::configuration::operator!=(const Configuration& left, const Configuration& right) noexcept
{
    return !(left == right);
}

constexpr bool core::build::configuration::operator==(const Configuration& left, ConfigurationType right) noexcept
{
    return left.type == right;
}

constexpr bool core::build::configuration::operator!=(const Configuration& left, ConfigurationType right) noexcept
{
    return !(left == right);
}
