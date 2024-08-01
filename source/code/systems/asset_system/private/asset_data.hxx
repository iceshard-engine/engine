#pragma once
#include <ice/asset.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    enum class AssetDataFlags : ice::u8
    {
        None,
        ReadOnly,
        Allocated,
    };

    struct AssetData
    {
        ice::UniquePtr<ice::AssetData> _next;
        ice::Allocator* _alloc;
        ice::AssetDataFlags _flags;
        ice::AssetState _state;

        ice::ualign _alignment;
        ice::usize _size;
        union
        {
            void* _readwrite;
            void const* _readonly;
        };
    };

    struct AllocatedAssetData : AssetData
    {
        ice::Allocator* _data_alloc;
    };

    inline auto asset_data_memory(ice::UniquePtr<ice::AssetData> const& data, ice::AssetState state) noexcept -> ice::Memory
    {
        if (data->_state == state && data->_flags == AssetDataFlags::Allocated)
        {
            return { data->_readwrite, data->_size, data->_alignment };
        }
        else if (data->_state > state)
        {
            return ice::asset_data_memory(data->_next, state);
        }
        return {};
    }

    inline auto asset_data_find(ice::UniquePtr<ice::AssetData> const& data, ice::AssetState state) noexcept -> ice::Data
    {
        if (data->_state == state)
        {
            return { data->_readonly, data->_size, data->_alignment };
        }
        else if (data->_state > state)
        {
            return ice::asset_data_find(data->_next, state);
        }
        return {};
    }

    template<typename T>
    inline void destroy_asset_data_entry(
        ice::AssetData* asset_data
    ) noexcept
    {
        T* const data = static_cast<T*>(asset_data);
        if constexpr(std::is_same_v<ice::AllocatedAssetData, T>)
        {
            data->_data_alloc->deallocate(data->_readwrite);
        }

        // Deallocate the entry
        data->_alloc->destroy(data);
    }

    inline auto create_asset_data_entry(
        ice::Allocator& alloc,
        ice::AssetState state,
        ice::Allocator& data_alloc,
        ice::Memory asset_memory
    ) noexcept -> ice::UniquePtr<ice::AssetData>
    {
        ice::AllocatedAssetData* data = alloc.create<ice::AllocatedAssetData>();
        data->_next = nullptr;
        data->_alloc = ice::addressof(alloc);
        data->_flags = AssetDataFlags::Allocated;
        data->_state = state;
        data->_alignment = asset_memory.alignment;
        data->_size = asset_memory.size;
        data->_readwrite = asset_memory.location;
        data->_data_alloc = ice::addressof(data_alloc);
        return ice::make_unique(
            ice::destroy_asset_data_entry<ice::AllocatedAssetData>,
            reinterpret_cast<ice::AssetData*>(data)
        );
    }

    inline auto create_asset_data_entry(
        ice::Allocator& alloc,
        ice::AssetState state,
        ice::Data asset_data
    ) noexcept -> ice::UniquePtr<ice::AssetData>
    {
        ice::AssetData* data = alloc.create<ice::AssetData>();
        data->_next = nullptr;
        data->_alloc = ice::addressof(alloc);
        data->_flags = AssetDataFlags::ReadOnly;
        data->_state = state;
        data->_alignment = asset_data.alignment;
        data->_size = asset_data.size;
        data->_readonly = asset_data.location;
        return ice::make_unique(ice::destroy_asset_data_entry<ice::AssetData>, data);
    }

} // namespace ice
