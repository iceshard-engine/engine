#pragma once
#include <ice/asset.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/resource_tracker.hxx>

namespace ice
{

    enum class AssetDataFlags : ice::u8
    {
        None = 0x00,
        Resource = 0x01,
        Metadata = 0x02,
        ReadOnly = 0x40,
        Allocated = 0x80,
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

    struct ResourceAssetData : AssetData
    {
        ice::ResourceHandle* _resource_handle;
    };

    inline auto asset_data_resource(ice::UniquePtr<ice::AssetData> const& data) noexcept -> ice::ResourceHandle*
    {
        ice::AssetData* dataptr = data.get();
        ice::ResourceHandle* result = nullptr;
        while(result == nullptr && dataptr != nullptr)
        {
            if (ice::has_any(dataptr->_flags, AssetDataFlags::Resource))
            {
                result = static_cast<ice::ResourceAssetData*>(dataptr)->_resource_handle;
            }
            else
            {
                dataptr = dataptr->_next.get();
            }
        }
        return result;
    }

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
        if (data == nullptr)
        {
            return {};
        }
        else if (data->_state == state)
        {
            return { data->_readonly, data->_size, data->_alignment };
        }
        else if (data->_state > state)
        {
            return ice::asset_data_find(data->_next, state);
        }
        return {};
    }

    inline auto asset_metadata_find(ice::UniquePtr<ice::AssetData> const& data, ice::Data& out_data) noexcept -> ice::Task<ice::Result>
    {
        if (ice::has_all(data->_flags, AssetDataFlags::Metadata))
        {
            if (ice::has_any(data->_flags, AssetDataFlags::Resource))
            {
                ice::ResourceAssetData* resdata = static_cast<ice::ResourceAssetData*>(data.get());
                co_return co_await ice::resource_meta(resdata->_resource_handle, out_data);
            }
            else // The data stored in this object is pure metadata
            {
                out_data = { data->_readonly, data->_size, data->_alignment };
                co_return S_Success;
            }
        }
        else if (data->_next != nullptr)
        {
            co_return co_await asset_metadata_find(data->_next, out_data);
        }
        co_return E_Fail;
    }

    inline auto asset_data_load(ice::UniquePtr<ice::AssetData> const& data, ice::ResourceTracker& tracker) noexcept -> ice::Task<ice::ResourceResult>
    {
        ICE_ASSERT_CORE(ice::has_all(data->_flags, AssetDataFlags::Resource));

        ice::ResourceAssetData* resdata = static_cast<ice::ResourceAssetData*>(data.get());
        ICE_ASSERT_CORE(resdata->_state == AssetState::Exists);

        co_return co_await tracker.load_resource(resdata->_resource_handle);
    }

    inline auto asset_metadata_load(ice::UniquePtr<ice::AssetData> const& asset_data, ice::Metadata& out_meta) noexcept -> ice::Task<ice::Result>
    {
        ice::Data data{};
        ice::Result result = co_await asset_metadata_find(asset_data, data);
        if (result == S_Ok)
        {
            if (data.location != nullptr)
            {
                out_meta = ice::meta_load(data);
            }
        }
        co_return result;
    }

    template<typename T> requires (std::is_base_of_v<ice::AssetData, T>)
    inline void destroy_asset_data_entry(T* asset_data) noexcept
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
    ) noexcept -> ice::UniquePtr<ice::AllocatedAssetData>
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
        return ice::make_unique(ice::destroy_asset_data_entry<ice::AllocatedAssetData>, data);
    }

    inline auto create_asset_data_entry(
        ice::Allocator& alloc,
        ice::AssetState state,
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::UniquePtr<ice::ResourceAssetData>
    {
        ice::ResourceAssetData* data = alloc.create<ice::ResourceAssetData>();
        data->_next = nullptr;
        data->_alloc = ice::addressof(alloc);
        data->_flags = AssetDataFlags::Resource | AssetDataFlags::Metadata | AssetDataFlags::ReadOnly;
        data->_state = state;
        data->_alignment = ice::ualign::b_default;
        data->_size = 0_B;
        data->_readonly = nullptr;
        data->_resource_handle = resource_handle;
        return ice::make_unique(ice::destroy_asset_data_entry<ice::ResourceAssetData>, data);
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
