#pragma once
#include <ice/asset.hxx>
#include <ice/memory.hxx>
#include <ice/allocator.hxx>

namespace ice::detail
{

    struct AssetObject
    {
        ice::AssetStatus status;
        ice::Data data;
        ice::Data metadata;
    };

    auto make_asset(
        AssetObject* object
    ) noexcept -> ice::Asset;

    auto make_empty_object(
        ice::Allocator& alloc,
        ice::AssetStatus status
    ) noexcept -> AssetObject*;

} // namespace ice
