#pragma once
#include <ice/allocator.hxx>
#include <ice/data.hxx>
#include <ice/memory.hxx>
#include <ice/memory.hxx>
#include <ice/asset.hxx>

namespace ice
{

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        virtual auto load(
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) noexcept -> ice::AssetStatus = 0;
    };

} // namespace ice
