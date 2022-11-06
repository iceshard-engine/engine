/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/os/windows.hxx>
#include <ice/string_types.hxx>
#include <ice/uri.hxx>

#include "resource_common_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class Resource_DllsWin32 final : public ice::Resource
    {
    public:
        Resource_DllsWin32(
            ice::HeapString<> origin_path,
            ice::String origin_name
        ) noexcept;

        ~Resource_DllsWin32() noexcept override = default;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

        auto metadata() const noexcept -> ice::Metadata const& override;

    private:
        ice::HeapString<> _origin_path;
        ice::String _origin_name;

        ice::URI _uri;
    };

    auto create_resource_from_dll_path(
        ice::Allocator& alloc,
        ice::WString file_path
    ) noexcept -> ice::Resource*;

} // namespace ice

#endif // #if ISP_WINDOWS
