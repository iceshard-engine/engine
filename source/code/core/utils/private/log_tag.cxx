/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_internal.hxx"

namespace ice
{

    void log_tag_register(ice::LogTagDefinition tag_def) noexcept
    {
        ice::detail::fn_register_log_tag(tag_def);
    }

    void log_tag_enable(ice::LogTag tag, bool enabled) noexcept
    {
        ice::detail::fn_enable_log_tag(tag, enabled);
    }

    namespace detail
    {

        void default_register_tag_fn(ice::LogTagDefinition tag_def) noexcept
        {
            ice::detail::internal_log_state->register_tag(tag_def);
        }

        void default_enable_tag_fn(ice::LogTag tag, bool enabled) noexcept
        {
            ice::detail::internal_log_state->enable_tag(tag, enabled);
        }

        void uninitialized_register_tag_fn(ice::LogTagDefinition /*log_def*/) noexcept { }
        void uninitialized_enable_tag_fn(ice::LogTag /*tag*/, bool /*enabled*/) noexcept { }

    } // namespace detail


} // namespace ice

ice::detail::RegisterLogTagFn* ice::detail::fn_register_log_tag = ice::detail::uninitialized_register_tag_fn;
ice::detail::EnableLogTagFn* ice::detail::fn_enable_log_tag = ice::detail::uninitialized_enable_tag_fn;
