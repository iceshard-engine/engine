#include "asset_internal.hxx"

namespace ice::detail
{

    auto make_asset(AssetObject* object) noexcept -> ice::Asset
    {
        return static_cast<ice::Asset>(reinterpret_cast<uintptr_t>(object));
    }

    auto make_empty_object(
        ice::Allocator& alloc,
        ice::StringID_Arg name,
        ice::AssetStatus status,
        ice::Metadata metadata
    ) noexcept -> AssetObject*
    {
        return alloc.make<AssetObject>(
            name,
            status,
            ice::Data{ },
            metadata
        );
    }

    void destroy_object(
        ice::Allocator& alloc,
        AssetObject* object
    ) noexcept
    {
        if (object != nullptr)
        {
            alloc.destroy(object);
        }
    }

} // namespace ice::detail
