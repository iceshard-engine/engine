/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/shard.hxx>
#include <ice/shard_container.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/stringid.hxx>
#include <ice/span.hxx>
#include <ice/engine_types.hxx>

namespace ice
{

    struct Trait;

    using TraitFactoryFn = auto(ice::Allocator& alloc, void* userdata) noexcept -> ice::UniquePtr<ice::Trait>;
    using TraitTypeRegisterFn = void(ice::Allocator& alloc) noexcept;
    using TraitTypeUnregisterFn = void(ice::Allocator& alloc) noexcept;

    struct TraitDescriptor
    {
        ice::StringID name;
        ice::TraitFactoryFn* fn_factory;
        ice::TraitTypeRegisterFn* fn_register;
        ice::TraitTypeUnregisterFn* fn_unregister;
        ice::Span<ice::StringID const> required_dependencies;
        ice::Span<ice::StringID const> optional_dependencies;
        void* fn_factory_userdata = nullptr;
    };

    static constexpr ice::Shard Shard_TraitSetup = "event/trait/setup`void"_shard;
    static constexpr ice::Shard Shard_TraitActivate = "event/trait/activate`void"_shard;
    static constexpr ice::Shard Shard_TraitUpdate = "event/trait/update`void"_shard;
    static constexpr ice::Shard Shard_TraitDeactivate = "event/trait/deactivate`void"_shard;
    static constexpr ice::Shard Shard_TraitShutdown = "event/trait/shutdown`void"_shard;

    using TraitTaskFn = auto (ice::Trait::*)(ice::Shard) noexcept -> ice::Task<>;
    using TraitIndirectTaskFn = auto (*)(ice::Trait*, ice::Shard, void*) noexcept -> ice::Task<>;

    //struct TraitInterval
    //{
    //    ice::u32 interval;
    //};

    struct TraitTaskLauncher
    {
        virtual ~TraitTaskLauncher() noexcept = default;

        virtual void bind(ice::TraitIndirectTaskFn func) noexcept = 0;
        virtual void bind(ice::TraitIndirectTaskFn func, ice::ShardID event) noexcept = 0;

        template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
        void bind(ice::ShardID event = ice::Shard_Invalid.id) noexcept;
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
    void TraitTaskLauncher::bind(ice::ShardID event) noexcept
    {
        if (event == ice::Shard_Invalid.id)
        {
            this->bind(detail::trait_method_task_wrapper<MemberPtr>);
        }
        else
        {
            this->bind(detail::trait_method_task_wrapper<MemberPtr>, event);
        }
    }

    struct Trait
    {
        virtual ~Trait() noexcept = default;

        virtual void gather_tasks(ice::TraitTaskLauncher& task_launcher) noexcept { }

        virtual auto activate(ice::EngineWorldUpdate const& world_update) noexcept -> ice::Task<> { co_return; }
        virtual auto deactivate(ice::EngineWorldUpdate const& world_update) noexcept -> ice::Task<> { co_return; }
    };

    struct TraitArchive
    {
        virtual ~TraitArchive() noexcept = default;

        virtual void register_trait(ice::TraitDescriptor trait_descriptor) noexcept = 0;
        virtual auto trait(ice::StringID_Arg traitid) const noexcept -> ice::TraitDescriptor const* = 0;
    };

    auto create_default_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::TraitArchive>;

    static constexpr ice::StringID TraitID_GfxImageStorage = "trait.gfx-image-storage"_sid;
    static constexpr ice::StringID TraitID_GfxShaderStorage = "trait.gfx-shader-storage"_sid;

} // namespace ice
