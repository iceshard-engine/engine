/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "asset_entry.hxx"
#include <ice/asset_request.hxx>
#include <ice/mem_memory.hxx>
#include <coroutine>

namespace ice
{

    class AssetShelve;

    class AssetRequestAwaitable final : public ice::AssetRequest
    {
    public:
        AssetRequestAwaitable(
            ice::StringID_Arg asset_name,
            ice::AssetStateTransaction& transation
        ) noexcept;

        ~AssetRequestAwaitable() noexcept override = default;

        auto state() const noexcept -> ice::AssetState override;
        auto data() const noexcept -> ice::Data override;
        auto metadata() const noexcept -> ice::Data override;

        auto asset_name() const noexcept -> ice::StringID_Arg override;
        auto asset_definition() const noexcept -> ice::AssetCategoryDefinition const& override;
        // auto resource() const noexcept -> ice::Resource const& override;

        auto allocate(ice::usize size) const noexcept -> ice::Memory override;

        auto resolve(
            ice::AssetResolveData resolve_data
        ) noexcept -> ice::Asset override;

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<void> coro) noexcept;
        auto await_resume() const noexcept -> ice::Memory;

        AssetRequestAwaitable* _next;
        AssetRequestAwaitable* _prev;

        AssetRequestAwaitable* _chained;

    private:
        ice::StringID const _asset_name;
        ice::AssetShelve& _asset_shelve;
        ice::AssetStateTransaction& _transaction;
        ice::Memory _result_data;

        std::coroutine_handle<> _coroutine;
    };

} // namespace ice
