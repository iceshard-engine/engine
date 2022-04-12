#pragma once
#include <ice/asset.hxx>
#include <ice/memory.hxx>

namespace ice
{

    class Resource_v2;

    struct ResourceHandle;

    struct AssetHandle
    {
    };

    struct AssetEntry : AssetHandle
    {
        ice::ResourceHandle* resource_handle;
        ice::Resource_v2 const* resource;
        ice::AssetState state;
        ice::Data data;
        ice::Memory data_baked;
        ice::Memory data_loaded;
        ice::Memory data_runtime;
    };

} // namespace ice
