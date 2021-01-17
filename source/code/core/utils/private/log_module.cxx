#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>
#include <ice/stringid.hxx>

#include "log_internal.hxx"

namespace ice
{

    namespace detail
    {

        struct LogAPI
        {
            RegisterLogTagFn** reg_log_tag_fn = &ice::detail::register_log_tag_fn;
            LogFn** log_fn = &ice::detail::log_fn;
            AssertFn** assert_fn = &ice::detail::assert_fn;
        };

    } // namespace detail

    bool get_log_api(
        ice::StringID_Hash name,
        ice::u32 version,
        void** api_ptr
    ) noexcept
    {
        static detail::LogAPI local_log_api{ };

        if (name == "ice.logger"_sid_hash && version == 1)
        {
            *api_ptr = &local_log_api;
            return true;
        }
        return false;
    }

    void initialize_log_module(
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept
    {
        detail::LogAPI* new_api = nullptr;
        if (api->fn_get_module_api(ctx, "ice.logger"_sid_hash, 1, reinterpret_cast<void**>(&new_api)))
        {
            detail::LogAPI const current_api{ };
            *current_api.reg_log_tag_fn = *new_api->reg_log_tag_fn;
            *current_api.log_fn = *new_api->log_fn;
            *current_api.assert_fn = *new_api->assert_fn;
        }
    }

    void load_log_module(ice::Allocator* alloc, ice::ModuleNegotiatorContext* ctx, ice::ModuleNegotiator* api) noexcept
    {
        detail::internal_log_state = alloc->make<detail::LogState>(*alloc);
        detail::LogAPI const current_api{ };
        *current_api.log_fn = ice::detail::default_log_fn;
        *current_api.reg_log_tag_fn = ice::detail::default_register_tag_fn;
        *current_api.assert_fn = ice::detail::default_assert_fn;

        api->fn_register_module(ctx, "ice.logger"_sid_hash, get_log_api);
    }

    void unload_log_module(
        ice::Allocator* alloc
    ) noexcept
    {
        alloc->destroy(detail::internal_log_state);
    }

} // namespace ice
