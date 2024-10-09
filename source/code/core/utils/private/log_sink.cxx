#include "log_internal.hxx"

namespace ice::detail
{

    auto default_register_sink_fn(ice::LogSinkFn fn_sink, void* userdata) noexcept -> ice::LogSinkID
    {
        return ice::detail::internal_log_state->register_sink(fn_sink, userdata);
    }

    void default_unregister_sink_fn(ice::LogSinkID sinkid) noexcept
    {
        ice::detail::internal_log_state->unregister_sink(sinkid);
    }

    auto uninitialized_register_sink_fn(ice::LogSinkFn /*fn_sin*/, void* /*userdata*/) noexcept -> ice::LogSinkID
    {
        return {};
    }

    void uninitialized_unregister_sink_fn(ice::LogSinkID /*sinkid*/) noexcept
    {
    }

} // namespace ice::detail

ice::detail::RegisterLogSinkFn* ice::detail::fn_register_log_sink = ice::detail::uninitialized_register_sink_fn;
ice::detail::UnregisterLogSinkFn* ice::detail::fn_unregister_log_sink = ice::detail::uninitialized_unregister_sink_fn;
