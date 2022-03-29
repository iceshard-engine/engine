#pragma once
#include <ice/asset_request.hxx>
#include <ice/memory.hxx>
#include <coroutine>

namespace ice
{

    class AssetShelve;

    struct AssetEntry;

    class AssetRequestAwaitable final : public ice::AssetRequest
    {
    public:
        AssetRequestAwaitable(
            ice::StringID_Arg asset_name,
            ice::AssetShelve& shelve,
            ice::AssetEntry const* entry,
            ice::AssetState requested_state
        ) noexcept;

        ~AssetRequestAwaitable() noexcept override = default;

        auto state() const noexcept -> ice::AssetState override;
        auto data() const noexcept -> ice::Data override;

        auto resource() const noexcept -> ice::Resource_v2 const& override;
        auto asset_definition() const noexcept -> ice::AssetTypeDefinition const& override;

        auto allocate(ice::u32 size) const noexcept -> ice::Memory override;

        void resolve(
            ice::AssetRequest::Result result,
            ice::Memory memory
        ) noexcept override;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<void> coro) noexcept;
        auto await_resume() const noexcept -> ice::Memory;

        AssetRequestAwaitable* _next;
        AssetRequestAwaitable* _prev;

    private:
        ice::StringID const _asset_name;
        ice::AssetShelve& _asset_shelve;
        ice::AssetEntry const* _asset_entry;
        ice::AssetState _requested_state;
        ice::Memory _result_data;

        std::coroutine_handle<> _coroutine;
    };

} // namespace ice
