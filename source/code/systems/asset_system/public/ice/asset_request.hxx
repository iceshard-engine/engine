#pragma once
#include <ice/base.hxx>
#include <ice/memory.hxx>
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>

namespace ice
{

    class Resource_v2;

    struct AssetTypeDefinition;

    class AssetRequest
    {
    public:
        enum class Result
        {
            Error,
            Success
        };

        virtual ~AssetRequest() noexcept = default;

        virtual auto state() const noexcept -> ice::AssetState = 0;
        virtual auto data() const noexcept -> ice::Data = 0;

        virtual auto resource() const noexcept -> ice::Resource_v2 const& = 0;
        virtual auto asset_definition() const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual auto allocate(ice::u32 size) const noexcept -> ice::Memory = 0;

        virtual void resolve(
            ice::AssetRequest::Result result,
            ice::Memory memory
        ) noexcept = 0;
    };

} // namespace ice
