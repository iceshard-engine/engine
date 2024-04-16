/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "resource_provider_filesystem.hxx"

namespace ice
{

    class FileSystemResourceProvider::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(ice::HashMap<ice::FileSystemResource*> const& resources) noexcept;
        ~DevUI() noexcept override;

        bool build_mainmenu(ice::DevUIWidgetState& state) noexcept override;

        void build_content() noexcept override;
        void build_resources_table(ice::u32 idx_start, ice::u32 idx_end) noexcept;

    private:
        ice::HashMap<ice::FileSystemResource*> const& _resources;
    };

} // namespace ice
