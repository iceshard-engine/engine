#pragma once
#include <ice/world/world_types.hxx>

namespace ice::detail
{

    struct TraitTaskTracker
    {
        virtual ~TraitTaskTracker() noexcept = default;
        virtual auto report_resume(ice::u32 id) noexcept -> ice::u32 = 0;
        virtual auto report_suspend(ice::u32 id) noexcept -> ice::u32 = 0;
        virtual void report_finish(ice::u32 id) noexcept = 0;
    };


    template<auto MethodPtr>
    static auto trait_method_task_wrapper(
        ice::Trait* self,
        ice::Shard sh,
        ice::detail::TraitTaskTracker* tracker
    ) noexcept -> ice::Task<>
    {
        using member_t = decltype(MethodPtr);
        using class_t = ice::member_class_type_t<member_t>;
        using result_t = ice::member_result_type_t<member_t>;
        using args_t = typename ice::member_info<member_t>::argument_types;
        constexpr ice::ucount args_count = ice::member_info<member_t>::argument_count;

        static_assert(
            std::is_same_v<result_t, ice::Task<>>,
            "Only ice::Task<> methods with one argument or less are allowed!"
        );

        if constexpr (args_count >= 1)
        {
            if constexpr (std::is_reference_v<std::tuple_element_t<0, args_t>>)
            {
                using ArgType = ice::clear_type_t<std::tuple_element_t<0, args_t>>*;
                ICE_ASSERT(
                    ice::Constant_ShardPayloadID<ArgType> == sh.id.payload,
                    "Shard payload ID incompatible with the argument. {} != {}",
                    ice::Constant_ShardPayloadID<ArgType>.value, sh.id.payload.value
                );

                ArgType shard_value = nullptr;
                [[maybe_unused]]
                bool const valid_value = ice::shard_inspect(sh, shard_value);
                ICE_ASSERT(valid_value, "Invalid value stored in Shard!");

                static auto method_caller = []<std::size_t... Idx>(
                    class_t* self,
                    auto&& argument,
                    std::index_sequence<Idx...>
                ) noexcept -> ice::Task<>
                {
                    co_await (self->*MethodPtr)(
                        ice::detail::map_task_arg<std::tuple_element_t<Idx, args_t>, std::tuple_element_t<0, args_t>>(argument)...
                    );
                };

                co_await method_caller(static_cast<class_t*>(self), *shard_value, std::make_index_sequence<args_count>());
            }
            else
            {
                using ArgType = std::tuple_element_t<0, args_t>;
                ICE_ASSERT(
                    ice::Constant_ShardPayloadID<ArgType> == sh.id.payload,
                    "Shard payload ID incompatible with the argument. {} != {}",
                    ice::Constant_ShardPayloadID<ArgType>.value, sh.id.payload.value
                );

                ArgType shard_value{ };
                [[maybe_unused]]
                bool const valid_value = ice::shard_inspect(sh, shard_value);
                ICE_ASSERT(valid_value, "Invalid value stored in Shard!");
                co_await(static_cast<class_t*>(self)->*MethodPtr)(shard_value);
            }
        }
        else
        {
            co_await (static_cast<class_t*>(self)->*MethodPtr)();
        }
        co_return;
    }

    template<auto MethodPtr, typename DataType>
    static auto trait_method_task_wrapper(
        ice::Trait* self,
        ice::Shard sh,
        ice::detail::TraitTaskTracker* tracker
    ) noexcept -> ice::Task<>
    {
        using member_t = decltype(MethodPtr);
        using class_t = ice::member_class_type_t<member_t>;
        using result_t = ice::member_result_type_t<member_t>;
        using args_t = typename ice::member_info<member_t>::argument_types;
        constexpr ice::ucount args_count = ice::member_info<member_t>::argument_count;

        static_assert(
            std::is_same_v<result_t, ice::Task<>> && args_count >= 1,
            "Only ice::Task<> methods with one argument or less are allowed!"
        );

        if constexpr (std::is_reference_v<DataType>)
        {
            using ArgType = ice::clear_type_t<DataType>*;
            ICE_ASSERT(
                ice::Constant_ShardPayloadID<ArgType> == sh.id.payload,
                "Shard payload ID incompatible with the argument. {} != {}",
                ice::Constant_ShardPayloadID<ArgType>.value, sh.id.payload.value
            );

            ArgType shard_value = nullptr;
            [[maybe_unused]]
            bool const valid_value = ice::shard_inspect(sh, shard_value);
            ICE_ASSERT(valid_value, "Invalid value stored in Shard!");

            static auto method_caller = []<std::size_t... Idx>(
                class_t* self,
                auto&& argument,
                std::index_sequence<Idx...>
            ) noexcept -> ice::Task<>
            {
                co_await (self->*MethodPtr)(
                    ice::detail::ArgMapper<std::tuple_element_t<Idx, args_t>>::select(argument)...
                );
            };

            co_await method_caller(static_cast<class_t*>(self), *shard_value, std::make_index_sequence<args_count>());
        }
        else
        {
            using ArgType = DataType;
            ICE_ASSERT(
                ice::Constant_ShardPayloadID<ArgType> == sh.id.payload,
                "Shard payload ID incompatible with the argument. {} != {}",
                ice::Constant_ShardPayloadID<ArgType>.value, sh.id.payload.value
            );

            ArgType shard_value{ };
            [[maybe_unused]]
            bool const valid_value = ice::shard_inspect(sh, shard_value);
            ICE_ASSERT(valid_value, "Invalid value stored in Shard!");
            co_await(static_cast<class_t*>(self)->*MethodPtr)(shard_value);
        }
        co_return;
    }

} // namespace ice::detail
