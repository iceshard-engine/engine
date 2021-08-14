#pragma once
#include <ice/asset.hxx>
#include <ice/memory.hxx>
#include <ice/allocator.hxx>
#include <ice/resource_meta.hxx>

namespace ice::detail
{

    struct AssetObject
    {
        ice::StringID const name;
        ice::AssetStatus status;
        ice::Data data;
        ice::Metadata metadata;
    };

    auto make_asset(
        AssetObject* object
    ) noexcept -> ice::Asset;

    auto make_empty_object(
        ice::Allocator& alloc,
        ice::StringID_Arg name,
        ice::AssetStatus status,
        ice::Metadata metadata
    ) noexcept -> AssetObject*;

} // namespace ice
