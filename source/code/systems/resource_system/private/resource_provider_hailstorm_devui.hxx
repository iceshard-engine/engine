/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "resource_provider_hailstorm.hxx"

#include <ice/string/heap_string.hxx>
#include <ice/devui_widget.hxx>

namespace ice
{

    class HailStormResourceProvider::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(
            ice::HeapString<> name,
            ice::HailStormResourceProvider const& provider
        ) noexcept;
        ~DevUI() noexcept override;

        bool build_mainmenu(ice::DevUIWidgetState& state) noexcept override;

        void build_content() noexcept override;
        void build_chunk_table() noexcept;
        void build_resources_table() noexcept;

    private:
        ice::HeapString<> const _name;
        ice::HailStormResourceProvider const& _provider;
    };

} // namespace ice
