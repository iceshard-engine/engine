#pragma once
#include <ice/data.hxx>
#include <ice/string_types.hxx>
#include <ice/resource_types.hxx>

namespace ice
{

    struct Metadata;

    class Resource_v2
    {
    public:
        virtual ~Resource_v2() noexcept = default;

        virtual auto uri() const noexcept -> ice::URI const& = 0;

        virtual auto name() const noexcept -> ice::Utf8String = 0;
        virtual auto origin() const noexcept -> ice::Utf8String = 0;

        virtual auto metadata() const noexcept -> ice::Metadata const& = 0;
    };

    constexpr auto operator|(
        ice::ResourceStatus left,
        ice::ResourceStatus right
    ) noexcept -> ice::ResourceStatus
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<ice::ResourceStatus>(left_val | right_val);
    }

    constexpr auto operator|=(
        ice::ResourceStatus& left,
        ice::ResourceStatus right
    ) noexcept -> ice::ResourceStatus&
    {
        left = left | right;
        return left;
    }

    constexpr auto operator&(
        ice::ResourceStatus left,
        ice::ResourceStatus right
    ) noexcept -> ice::ResourceStatus
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<ice::ResourceStatus>(left_val & right_val);
    }

    constexpr auto operator&=(
        ice::ResourceStatus& left,
        ice::ResourceStatus right
    ) noexcept -> ice::ResourceStatus&
    {
        left = left & right;
        return left;
    }

    constexpr bool has_flag(ice::ResourceStatus value, ice::ResourceStatus searched_mask) noexcept
    {
        return (value & searched_mask) == searched_mask;
    }

} // ice::res_v2
