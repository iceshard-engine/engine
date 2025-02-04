/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_types.hxx>

namespace ice
{

    struct ResourceHandle final
    {
        ice::Resource* resource = nullptr;

        ResourceHandle() noexcept = default;
        explicit ResourceHandle(ice::Resource* resource) noexcept;
        ~ResourceHandle() noexcept;

        ResourceHandle(ResourceHandle&& other) noexcept;
        ResourceHandle(ResourceHandle const& other) noexcept;
        auto operator=(ResourceHandle&& other) noexcept -> ResourceHandle&;
        auto operator=(ResourceHandle const& other) noexcept -> ResourceHandle&;

        inline constexpr auto operator->() noexcept -> ice::Resource* { return resource; }
        inline constexpr auto operator->() const noexcept -> ice::Resource const* { return resource; }

        inline constexpr bool valid() const noexcept { return resource != nullptr; }
        inline constexpr operator ice::Resource* () const noexcept { return this->resource; }
    };

    inline static ice::ResourceHandle const ResourceHandle_Invalid{};

} // namespace ice
