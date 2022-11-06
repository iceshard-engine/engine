/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/mem_memory.hxx>

namespace ice
{

    class Resource;
    class AssetRequestAwaitable;

    struct ResourceHandle;

    struct AssetHandle
    {
    };

    struct AssetEntry : AssetHandle
    {
        ice::ResourceHandle* resource_handle;
        ice::Resource const* resource;
        ice::AssetState state;
        ice::Data data;
        ice::Memory data_baked;
        ice::Memory data_loaded;
        ice::Memory data_runtime;

        ice::AssetRequestAwaitable* request_awaitable;
    };

} // namespace ice
