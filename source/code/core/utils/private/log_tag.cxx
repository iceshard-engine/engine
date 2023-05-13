/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "log_internal.hxx"

namespace ice
{

    void register_log_tag(ice::LogTagDefinition tag_def) noexcept
    {
        ice::detail::register_log_tag_fn(tag_def);
    }

    namespace detail
    {

        void default_register_tag_fn(ice::LogTagDefinition tag_def) noexcept
        {
            ice::detail::internal_log_state->register_tag(tag_def);
        }

        void uninitialized_register_tag_fn(
            ice::LogTagDefinition /*log_def*/
        ) noexcept
        {
        }

    } // namespace detail


} // namespace ice

ice::detail::RegisterLogTagFn* ice::detail::register_log_tag_fn = ice::detail::uninitialized_register_tag_fn;
