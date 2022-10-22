/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_app.hxx>

namespace ice::platform
{

    class SDL2_Container final : public ice::platform::Container
    {
    public:
        SDL2_Container(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::platform::App> app
        ) noexcept;

        ~SDL2_Container() noexcept override;

        auto run() noexcept -> ice::i32 override;

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<ice::platform::App> _app;

        bool _request_quit = false;
    };

} // namespace ice::platform
