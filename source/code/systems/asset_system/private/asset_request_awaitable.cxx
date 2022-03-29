#include "asset_request_awaitable.hxx"
#include "asset_shelve.hxx"
#include "asset_entry.hxx"

namespace ice
{

    AssetRequestAwaitable::AssetRequestAwaitable(
        ice::StringID_Arg asset_name,
        ice::AssetShelve& shelve,
        ice::AssetEntry const* entry,
        ice::AssetState requested_state
    ) noexcept
        : _next{ nullptr }
        , _prev{ nullptr }
        , _asset_shelve{ shelve }
        , _asset_name{ asset_name }
        , _asset_entry{ entry }
        , _requested_state{ requested_state }
    {
    }

    auto AssetRequestAwaitable::state() const noexcept -> ice::AssetState
    {
        return _asset_entry->state;
    }

    auto AssetRequestAwaitable::data() const noexcept -> ice::Data
    {
        return _asset_entry->data;
    }

    auto AssetRequestAwaitable::resource() const noexcept -> ice::Resource_v2 const&
    {
        return *_asset_entry->resource;
    }

    auto AssetRequestAwaitable::asset_definition() const noexcept -> ice::AssetTypeDefinition const&
    {
        return _asset_shelve.definition;
    }

    auto AssetRequestAwaitable::allocate(ice::u32 size) const noexcept -> ice::Memory
    {
        return ice::Memory{
            .location = _asset_shelve.asset_allocator().allocate(size),
            .size = size,
            .alignment = 4
        };
    }

    void AssetRequestAwaitable::resolve(
        ice::AssetRequest::Result result,
        ice::Memory memory
    ) noexcept
    {
        if (result == AssetRequest::Result::Error)
        {
            _asset_shelve.asset_allocator().deallocate(memory.location);
        }
        else
        {
            _result_data = memory;
        }

        _coroutine.resume();
    }

    void AssetRequestAwaitable::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _coroutine = coro;
        _asset_shelve.append_request(this, _requested_state);
    }

    auto AssetRequestAwaitable::await_resume() const noexcept -> ice::Memory
    {
        return _result_data;
    }

} // namespace ice
