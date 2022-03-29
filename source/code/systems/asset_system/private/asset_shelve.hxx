#pragma once
#include <ice/allocator.hxx>
#include <ice/asset_type_archive.hxx>
#include <atomic>

#include "asset_entry.hxx"

namespace ice
{

    class AssetRequestAwaitable;

    class AssetShelve final
    {
    public:
        AssetShelve(
            ice::Allocator& alloc,
            ice::AssetTypeDefinition const& definition
        ) noexcept;

        ~AssetShelve() noexcept;

        auto asset_allocator() noexcept -> ice::Allocator&;

        auto select(
            ice::StringID_Arg name
        ) noexcept -> ice::AssetEntry*;

        auto select(
            ice::StringID_Arg name
        ) const noexcept -> ice::AssetEntry const*;

        auto store(
            ice::StringID_Arg name,
            ice::ResourceHandle* resource_handle,
            ice::Resource_v2 const* resource,
            ice::AssetState state,
            ice::Data data
        ) noexcept -> ice::AssetEntry*;

        void append_request(
            ice::AssetRequestAwaitable* request,
            ice::AssetState state
        ) noexcept;

        auto aquire_request(
            ice::AssetState state
        ) noexcept -> ice::AssetRequestAwaitable*;

        ice::AssetTypeDefinition const& definition;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::AssetEntry*> _asset_resources;

        std::atomic<ice::AssetRequestAwaitable*> _new_requests[3];
        std::atomic<ice::AssetRequestAwaitable*> _reversed_requests[3];
    };

} // namespace ice

