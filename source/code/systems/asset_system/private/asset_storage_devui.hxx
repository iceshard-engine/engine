/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "asset_storage.hxx"
#include "asset_shelve_devui.hxx"

namespace ice
{

    class DefaultAssetStorage::DevUI : public ice::DevUIWidget
    {
    public:
        DevUI(
            ice::Allocator& alloc,
            ice::DefaultAssetStorage& storage,
            ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> shelves
        ) noexcept;
        ~DevUI() noexcept;

        void build_content() noexcept override;

    private:
        ice::DefaultAssetStorage& _storage;
        ice::Array<ice::UniquePtr<ice::AssetShelve::DevUI>> _shelves;
    };

} // namespace ice
