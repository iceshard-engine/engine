#pragma once
#include <core/base.hxx>

namespace iceshard::input
{

    struct DeviceInputData
    {
        uint8_t type;
        uint8_t bytes : 4;
        uint8_t count : 4;
    };

    namespace detail
    {

        template<typename T>
        static constexpr DeviceInputData input_data_info{ };

        template<>
        static constexpr DeviceInputData input_data_info<void>{
            .type = 0xff,
            .bytes = 0
        };

        template<>
        static constexpr DeviceInputData input_data_info<bool>{
            .type = 0x1,
            .bytes = 1
        };

        template<>
        static constexpr DeviceInputData input_data_info<float>{
            .type = 0x2,
            .bytes = 4
        };

        template<>
        static constexpr DeviceInputData input_data_info<int8_t>{
            .type = 0x3,
            .bytes = 1
        };

        template<>
        static constexpr DeviceInputData input_data_info<uint8_t>{
            .type = 0x3,
            .bytes = 1
        };

        template<>
        static constexpr DeviceInputData input_data_info<int16_t>{
            .type = 0x4,
            .bytes = 2
        };

        template<>
        static constexpr DeviceInputData input_data_info<uint16_t>{
            .type = 0x4,
            .bytes = 2
        };

        template<>
        static constexpr DeviceInputData input_data_info<int32_t>{
            .type = 0x5,
            .bytes = 4
        };

        template<>
        static constexpr DeviceInputData input_data_info<uint32_t>{
            .type = 0x5,
            .bytes = 4
        };

        template<typename T = void, typename...>
        struct first_type_from_list
        {
            using type = T;
        };

    } // namespace detail

} // namespace iceshard::input
