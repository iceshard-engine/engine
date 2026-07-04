#pragma once
#include <ice/i18n_reference.hxx>
#include <fmt/format.h>

namespace ice
{

    struct I18NString;

    class I18NResolver
    {
    public:
        virtual ~I18NResolver() noexcept = default;

        virtual auto resolve(ice::I18NReference key) const noexcept -> ice::String = 0;
        virtual auto resolve(ice::I18NReference key, fmt::format_args const& args) const noexcept -> ice::String = 0;
        virtual void resolve(ice::I18NString& inout_text, ice::I18NReference const& ref) const noexcept { }
    };

} // namespace ice
