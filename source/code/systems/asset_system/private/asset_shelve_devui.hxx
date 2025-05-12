/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "asset_shelve.hxx"

namespace ice
{

    class AssetShelve::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(ice::AssetShelve& shelve) noexcept;
        ~DevUI() noexcept;

        void build_content() noexcept override;

    private:
        ice::AssetShelve& _shelve;
    };

} // namespace ice
