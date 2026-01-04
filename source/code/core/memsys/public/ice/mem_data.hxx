/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_info.hxx>
#include <ice/mem_arithmetic.hxx>

namespace ice
{

    template<typename T>
    concept HasDataRepresentation = requires(T t) {
        { t.operator ice::Data() } -> std::convertible_to<ice::Data>;
    };

    struct Data
    {
        void const* location;
        ice::usize size;
        ice::ualign alignment;
    };

    template<typename Type>
    inline auto data_view(Type const& var) noexcept
    {
        if constexpr (HasDataRepresentation<Type>)
        {
            return (ice::Data) var;
        }
        else
        {
            return Data{
                .location = std::addressof(var),
                .size = ice::size_of<Type>,
                .alignment = ice::align_of<Type>
            };
        }
    }

    template<typename Type, ice::usize::base_type Size>
    constexpr auto data_view(Type const(&var)[Size]) noexcept
    {
        return Data{
            .location = std::addressof(var),
            .size = ice::size_of<Type> *Size,
            .alignment = ice::align_of<Type>
        };
    }

    inline auto ptr_add(ice::Data mem, ice::usize offset) noexcept -> ice::Data
    {
        ICE_ASSERT_CORE(mem.size >= offset);
        return Data{
            .location = ice::ptr_add(mem.location, offset),
            .size = {mem.size.value - offset.value},
            .alignment = mem.alignment
        };
    }

    namespace data
    {

        constexpr auto with_highest_alignment(ice::Data data) noexcept -> ice::Data
        {
            ice::u32 align_value = static_cast<std::underlying_type_t<ice::ualign>>(data.alignment);

            // We check all bits on the next alignment. Ex.: (8 << 1) - 1 = 15 [0x00ff]
            while ((std::bit_cast<ice::uptr>(data.location) & ((align_value << 1) - 1)) == 0)
            {
                // Increase alignment
                align_value <<= 1;

                // If alignment reaches 128 we stop here.
                if (align_value == static_cast<std::underlying_type_t<ice::ualign>>(ice::ualign::b_256))
                {
                    break;
                }
            }

            data.alignment = static_cast<ice::ualign>(align_value);
            return data;
        }

        template<typename T>
            requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        inline auto read_raw(ice::Data source, T& out_value) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value = *reinterpret_cast<T const*>(source.location);
            source.location = ice::ptr_add(source.location, ice::size_of<T>);
            source.size = ice::usize{ source.size.value - ice::size_of<T>.value };
            source.alignment = ice::align_of<T>;
            return source;
        }

        template<typename T, ice::u64 Size>
            requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        inline auto read_raw_array(ice::Data source, T(&out_array)[Size]) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            ice::memcpy(out_array, source.location, sizeof(T) * Size);
            source.location = ice::ptr_add(source.location, ice::size_of<T> * Size);
            source.size = ice::usize{ source.size.value - ice::size_of<T>.value * Size };
            source.alignment = ice::align_of<T>;
            return source;
        }

        template<typename T>
            requires (std::is_trivially_copyable_v<T>)
        inline auto read_raw(ice::Data source, T*& out_value_ptr) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value_ptr = reinterpret_cast<T const*>(source.location);
            source.location = out_value_ptr + 1;
            source.size = ice::usize{ source.size.value - ice::size_of<T>.value };
            source.alignment = ice::align_of<T>;
            return source;
        }

        template<typename T, typename OffsetType = ice::usize::base_type>
            requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        inline auto read_offset(ice::Data source, T& out_value) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<OffsetType>);
            OffsetType offset;
            read_raw(source, offset);
            source.location = ice::ptr_add(source.location, { offset });
            source.size = ice::usize{ source.size.value - offset };
            source = with_highest_alignment(source);

            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value = *reinterpret_cast<T const*>(source.location);
            source.location = &out_value + 1;
            source.size = ice::usize{ source.size.value - ice::size_of<T>.value };
            source.alignment = ice::align_of<T>;
            return source;
        }

        template<typename T, typename OffsetType = ice::usize::base_type>
            requires (std::is_trivially_copyable_v<T>)
        inline auto read_offset(ice::Data source, T*& out_value_ptr) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<OffsetType>);
            OffsetType offset;
            read_raw(source, offset);
            source.location = ice::ptr_add(source.location, { offset });
            source.size = ice::usize{ source.size.value - offset };
            source = with_highest_alignment(source);

            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value_ptr = reinterpret_cast<T const*>(source.location);
            source.location = out_value_ptr + 1;
            source.size = ice::usize{ source.size.value - ice::size_of<T>.value };
            source.alignment = ice::align_of<T>;
            return source;
        }

    } // namespace data

} // namespace ice
