#pragma once
#include <ice/task_checkpoint.hxx>
#include <ice/world/world_types.hxx>

namespace ice
{

    static constexpr ice::ErrorCode E_TraitContextNotInitialized{ "E.1100:Worlds:World trait context was not initialized!" };

    struct TraitContext
    {
        virtual ~TraitContext() noexcept = default;

        virtual auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate = 0;
        virtual bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;
        virtual void unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;

        virtual auto bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result = 0;

        template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
        auto bind(ice::ShardID event = ice::Shard_Invalid.id) noexcept -> ice::Result;
    };

    using TraitTaskFn = auto (ice::Trait::*)(ice::Shard) noexcept -> ice::Task<>;
    using TraitIndirectTaskFn = auto (*)(ice::Trait*, ice::Shard, void*) noexcept -> ice::Task<>;

    struct TraitTaskBinding
    {
        ice::ShardID trigger_event;
        ice::TraitIndirectTaskFn procedure;
        ice::TraitTaskType task_type = ice::TraitTaskType::Frame;
        void* procedure_userdata;
    };

    namespace detail
    {

        template<auto MethodPtr>
        static auto trait_method_task_wrapper(ice::Trait* self, ice::Shard sh, void*) noexcept -> ice::Task<>
        {
            using member_t = decltype(MethodPtr);
            using class_t = ice::member_class_type_t<member_t>;
            using result_t = ice::member_result_type_t<member_t>;
            using args_t = typename ice::member_info<member_t>::argument_types;
            constexpr ice::ucount args_count = ice::member_info<member_t>::argument_count;

            static_assert(
                std::is_same_v<result_t, ice::Task<>> && args_count <= 1,
                "Only ice::Task<> methods with one argument or less are allowed!"
            );

            if constexpr (args_count == 1)
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
                    co_await(static_cast<class_t*>(self)->*MethodPtr)(*shard_value);
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

    } // namespace detail

    template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
    auto TraitContext::bind(ice::ShardID event) noexcept -> ice::Result
    {
        return this->bind(
            TraitTaskBinding{
                .trigger_event = event,
                .procedure = detail::trait_method_task_wrapper<MemberPtr>,
                .task_type = ice::TraitTaskType::Frame,
                .procedure_userdata = nullptr,
            }
        );
    }

} // namespace ice
