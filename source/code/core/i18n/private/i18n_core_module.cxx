#include <ice/i18n_core_module.hxx>
#include <ice/i18n_string.hxx>

namespace ice
{

    class I18NFallbackResolver final : public ice::I18NResolver
    {
    public:
        auto resolve(ice::I18NReference key) const noexcept -> ice::String override
        {
            return key.fallback();
        }

        auto resolve(ice::I18NReference key, fmt::format_args const& args) const noexcept -> ice::String override
        {
            return key.fallback();
        }

        void resolve(ice::I18NString& inout_text, ice::I18NReference const& ref) const noexcept override
        {
            // Nothing to do
        }
    };

    namespace detail
    {

        static auto v1_get_i18n_fallback_resolver() noexcept -> ice::I18NResolver const*
        {
            static I18NFallbackResolver const global_I18NFallbackResolver{};
            return &global_I18NFallbackResolver;
        }

        static ice::api::i18n::v1::FnGetI18NResolver fn_get_i18n_resolver = &v1_get_i18n_fallback_resolver;

    } // namespace detail

    void I18NCoreModule::v1_i18n_api(ice::api::i18n::v1::I18NCoreModuleAPI& api) noexcept
    {
        api.fn_get_i18n_resolver = ice::detail::v1_get_i18n_fallback_resolver;
    }

    bool I18NCoreModule::on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
    {
        return true;
    }

    void I18NCoreModule::on_unload(ice::Allocator& alloc) noexcept
    {
        // Restore the fallback resolver
        ice::detail::fn_get_i18n_resolver = ice::detail::v1_get_i18n_fallback_resolver;
    }

    void I18NCoreModule::init(const Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept
    {
        ice::api::i18n::v1::I18NCoreModuleAPI core_api;
        if (negotiator.query_api(core_api))
        {
            ice::detail::fn_get_i18n_resolver = core_api.fn_get_i18n_resolver;
        }
    }

    auto I18NCoreModule::resolve(ice::I18NReference const& reference) noexcept -> ice::String
    {
        return ice::detail::fn_get_i18n_resolver()->resolve(reference);
    }

    auto I18NCoreModule::resolve(
        ice::I18NReference const& reference,
        fmt::format_args const& fmt_args
    ) noexcept -> ice::String
    {
        return ice::detail::fn_get_i18n_resolver()->resolve(reference, fmt_args);
    }

    void I18NCoreModule::resolve(ice::I18NString& text, ice::I18NReference const& reference) noexcept
    {
        ice::detail::fn_get_i18n_resolver()->resolve(text, reference);
    }

} // namespace ice
