#pragma once
#include <ice/allocator.hxx>
#include <ice/data.hxx>
#include <ice/memory.hxx>
#include <ice/memory.hxx>
#include <ice/asset.hxx>

namespace ice
{

    class AssetSolver
    {
    public:
        virtual ~AssetSolver() noexcept = default;

        virtual auto find_asset(
            ice::AssetType type,
            ice::StringID_Hash asset_name_hash
        ) noexcept -> ice::Asset = 0;
    };

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        virtual auto load(
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) const noexcept -> ice::AssetStatus
        {
            return ice::AssetStatus::Invalid;
        }

        virtual auto load(
            ice::AssetSolver& asset_solver,
            ice::AssetType type,
            ice::Data data,
            ice::Allocator& alloc,
            ice::Memory& out_data
        ) const noexcept -> ice::AssetStatus
        {
            return load(type, data, alloc, out_data);
        }
    };

} // namespace ice
