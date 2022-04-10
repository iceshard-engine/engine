#pragma once
#include <ice/asset_type_archive.hxx>

namespace ice
{

    bool asset_image_oven(
        void*,
        ice::Allocator&,
        ice::ResourceTracker const&,
        ice::Resource_v2 const&,
        ice::Data,
        ice::Memory&
    ) noexcept;

    bool asset_image_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept;

} // namespace iceshard
