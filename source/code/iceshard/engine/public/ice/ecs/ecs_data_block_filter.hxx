/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_data_block.hxx>

namespace ice::ecs::detail
{

    template<typename T>
    concept FilterType = requires(T t, T const* const_param, T * mut_param) {
        { T::on_filter(const_param, const_param) } -> std::convertible_to<bool>;
        { T::on_setup(mut_param, const_param) } -> std::convertible_to<void>;
    };

    struct DataBlockFilter
    {
        bool enabled = false;

        ice::usize data_size = 0_B;

        using FilterFn = bool(*)(void const* block_data, void const* filter_param) noexcept;
        FilterFn fn_filter = nullptr;

        using DataSetupFn = void(*)(void* block_data, void const* filter_param) noexcept;
        DataSetupFn fn_data_setup = nullptr;

        struct QueryFilter
        {
            QueryFilter() noexcept
                : arch{ }
                , fn_filter{ nullptr }
                , filter_data{ }
            { }

            template<ice::ecs::detail::FilterType T>
            QueryFilter(T const& filter) noexcept
                : arch{ }
                , fn_filter{ (DataBlockFilter::FilterFn)T::on_filter }
                , filter_data{ }
            {
                ice::memcpy(filter_data, ice::addressof(filter), sizeof(T));
            }

            QueryFilter(QueryFilter const& other) noexcept
                : fn_filter{ other.fn_filter }
            {
                ice::memcpy(filter_data, other.filter_data, sizeof(filter_data));
            }

            auto operator=(QueryFilter const& other) noexcept -> QueryFilter&
            {
                if (ice::addressof(other) != this)
                {
                    arch = other.arch;
                    fn_filter = other.fn_filter;
                    ice::memcpy(filter_data, other.filter_data, sizeof(filter_data));
                }
                return *this;
            }

            bool check(ice::ecs::detail::DataBlock const* data_block) const noexcept
            {
                return fn_filter == nullptr || fn_filter(data_block->block_filter_data, filter_data);
            }

            auto next(ice::ecs::detail::DataBlock const* data_block) const noexcept
            {
                ICE_ASSERT_CORE(data_block != nullptr);
                if (fn_filter == nullptr)
                {
                    return data_block->next;
                }
                else
                {
                    ice::ecs::detail::DataBlock* result = data_block->next;
                    while (result != nullptr && check(result) == false)
                    {
                        result = result->next;
                    }
                    return result;
                }
            }

            ice::ecs::Archetype arch;
            FilterFn fn_filter;
            char filter_data[16];
        };
    };

    template<typename T> requires (FilterType<T>)
    auto create_data_block_filter() noexcept -> ice::ecs::detail::DataBlockFilter
    {
        return DataBlockFilter{
            .enabled = true,
            .data_size = ice::size_of<T>,
            .fn_filter = (DataBlockFilter::FilterFn)T::on_filter,
            .fn_data_setup = (DataBlockFilter::DataSetupFn)T::on_setup,
        };
    }

} // namespace ice::ecs::detail
