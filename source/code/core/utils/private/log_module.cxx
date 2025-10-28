/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/module.hxx>
#include <ice/module_negotiator.hxx>
#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>
#include <ice/stringid.hxx>

#include "log_internal.hxx"
#include "log_android.hxx"
#include "log_webasm.hxx"

namespace ice
{

    namespace detail
    {

        struct LogAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.logger"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            RegisterLogSinkFn** reg_log_sink_fn = &ice::detail::fn_register_log_sink;
            UnregisterLogSinkFn** unreg_log_sink_fn = &ice::detail::fn_unregister_log_sink;
            RegisterLogTagFn** reg_log_tag_fn = &ice::detail::fn_register_log_tag;
            EnableLogTagFn** ena_log_tag_fn = &ice::detail::fn_enable_log_tag;
            LogFn** log_fn = &ice::detail::log_fn;
            AssertFn** assert_fn = &ice::detail::assert_fn;
        };

        LogState* internal_log_state = nullptr;

    } // namespace detail

    bool get_log_api(
        ice::StringID_Hash name,
        ice::u32 version,
        ice::ModuleAPI* api_ptr
    ) noexcept
    {
        static detail::LogAPI local_log_api{ };

        if (name == "ice.logger"_sid_hash && version == 1)
        {
            api_ptr->api_ptr = &local_log_api;
            api_ptr->version = 1;
            api_ptr->priority = 100;
            return true;
        }
        return false;
    }

    void initialize_log_module(
        ice::ModuleNegotiatorAPIContext* ctx,
        ice::ModuleNegotiatorAPI* api
    ) noexcept
    {
    }

    void load_log_module(ice::Allocator* alloc, ice::ModuleNegotiatorAPIContext* ctx, ice::ModuleNegotiatorAPI* api) noexcept
    {
        detail::internal_log_state = alloc->create<detail::LogState>(*alloc);
        detail::LogAPI const current_api{ };
        if constexpr(ice::build::is_android)
        {
            *current_api.log_fn = ice::detail::android::logcat_message;
            *current_api.reg_log_sink_fn = ice::detail::default_register_sink_fn;
            *current_api.unreg_log_sink_fn = ice::detail::default_unregister_sink_fn;
            *current_api.reg_log_tag_fn = ice::detail::default_register_tag_fn;
            *current_api.ena_log_tag_fn = ice::detail::default_enable_tag_fn;
            *current_api.assert_fn = ice::detail::android::logcat_assert;
        }
        else if constexpr (ice::build::is_webapp)
        {
            *current_api.log_fn = ice::detail::webasm::console_message;
            *current_api.reg_log_sink_fn = ice::detail::default_register_sink_fn;
            *current_api.unreg_log_sink_fn = ice::detail::default_unregister_sink_fn;
            *current_api.reg_log_tag_fn = ice::detail::default_register_tag_fn;
            *current_api.ena_log_tag_fn = ice::detail::default_enable_tag_fn;
            *current_api.assert_fn = ice::detail::webasm::alert_assert;
        }
        else
        {
            *current_api.log_fn = ice::detail::default_log_fn;
            *current_api.reg_log_sink_fn = ice::detail::default_register_sink_fn;
            *current_api.unreg_log_sink_fn = ice::detail::default_unregister_sink_fn;
            *current_api.reg_log_tag_fn = ice::detail::default_register_tag_fn;
            *current_api.ena_log_tag_fn = ice::detail::default_enable_tag_fn;
            *current_api.assert_fn = ice::detail::default_assert_fn;
        }

        api->fn_register_api(ctx, "ice.logger"_sid_hash, get_log_api);
    }

    void unload_log_module(
        ice::Allocator* alloc
    ) noexcept
    {
        alloc->destroy(detail::internal_log_state);
    }

    void log_module_init(ice::Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept
    {
        detail::LogAPI new_api{ };
        if (negotiator.query_api(new_api))
        {
            detail::LogAPI const current_api{ };
            *current_api.reg_log_sink_fn = *new_api.reg_log_sink_fn;
            *current_api.reg_log_tag_fn = *new_api.reg_log_tag_fn;
            *current_api.ena_log_tag_fn = *new_api.ena_log_tag_fn;
            *current_api.log_fn = *new_api.log_fn;
            *current_api.assert_fn = *new_api.assert_fn;
        }
    }

    auto log_module_register_sink(LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID
    {
        static detail::LogAPI const current_api{ };
        return (*current_api.reg_log_sink_fn)(fn_sink, userdata);
    }

    void log_module_unregister_sink(ice::LogSinkID sinkid) noexcept
    {
        static detail::LogAPI const current_api{ };
        return (*current_api.unreg_log_sink_fn)(sinkid);
    }

    void LogModule::init(ice::Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept
    {
        log_module_init(alloc, negotiator);
    }

    bool LogModule::on_load(ice::Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept
    {
        load_log_module(&alloc, negotiator.negotiator_context, negotiator.negotiator_api);
        return true;
    }

    void LogModule::on_unload(ice::Allocator& alloc) noexcept
    {
        unload_log_module(&alloc);
    }

} // namespace ice
