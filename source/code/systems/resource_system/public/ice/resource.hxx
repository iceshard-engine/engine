#pragma once
#include <ice/uri.hxx>
#include <ice/data.hxx>

namespace ice
{

    class ResourceHandle;

    enum class ResourceStatus_v2 : ice::u32
    {
        Invalid = 0x00'01,
        Available = 0x00'02,
        Loaded = 0x00'04,
        Loading = 0x02'00,
        Unloading = 0x04'00,
    };

    class Resource_v2
    {
    public:
        virtual ~Resource_v2() noexcept = default;

        virtual auto uri() const noexcept -> ice::URI_v2 const& = 0;

        virtual auto name() const noexcept -> ice::Utf8String = 0;
        virtual auto origin() const noexcept -> ice::Utf8String = 0;

        virtual auto metadata() const noexcept -> ice::Metadata const& = 0;
    };

    constexpr auto operator|(
        ice::ResourceStatus_v2 left,
        ice::ResourceStatus_v2 right
    ) noexcept -> ice::ResourceStatus_v2
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<ice::ResourceStatus_v2>(left_val | right_val);
    }

    constexpr auto operator|=(
        ice::ResourceStatus_v2& left,
        ice::ResourceStatus_v2 right
    ) noexcept -> ice::ResourceStatus_v2&
    {
        left = left | right;
        return left;
    }

    constexpr auto operator&(
        ice::ResourceStatus_v2 left,
        ice::ResourceStatus_v2 right
    ) noexcept -> ice::ResourceStatus_v2
    {
        ice::u32 const left_val = static_cast<ice::u32>(left);
        ice::u32 const right_val = static_cast<ice::u32>(right);
        return static_cast<ice::ResourceStatus_v2>(left_val & right_val);
    }

    constexpr auto operator&=(
        ice::ResourceStatus_v2& left,
        ice::ResourceStatus_v2 right
    ) noexcept -> ice::ResourceStatus_v2&
    {
        left = left & right;
        return left;
    }

    constexpr bool has_flag(ice::ResourceStatus_v2 value, ice::ResourceStatus_v2 searched_mask) noexcept
    {
        return (value & searched_mask) == searched_mask;
    }

} // ice::res_v2
