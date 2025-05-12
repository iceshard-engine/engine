/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "iceshard_world_manager.hxx"

#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_module.hxx>
#include <ice/devui_widget.hxx>

namespace ice
{

    class IceshardWorldManager::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(
            ice::Allocator& alloc,
            ice::DevUIWidgetInfo const& info,
            ice::IceshardWorldManager& manager
        ) noexcept;

        void build_content() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::IceshardWorldManager& _manager;

        struct Entry
        {
            ice::i32 status;
        };

        ice::Array<Entry> _entries;
        ice::u32 _selected;
    };

} // namespace ice
