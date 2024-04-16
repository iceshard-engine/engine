/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widget.hxx>
#include "resource_tracker.hxx"

namespace ice
{

    class ResourceTrackerImplementation::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(
            ice::Allocator& alloc,
            ice::ResourceTrackerImplementation& tracker
        ) noexcept;
        ~DevUI() noexcept override;

        void build_content() noexcept;

    protected:
        void build_resource_view() noexcept;

    private:
        ice::ResourceTrackerImplementation& _tracker;
    };

} // namespace ice
