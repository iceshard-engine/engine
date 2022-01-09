#include <ice/resource_tracker.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_provider_action.hxx>
#include <ice/resource.hxx>
#include <ice/resource_action.hxx>
#include <ice/resource.hxx>
#include <ice/pod/hash.hxx>
#include <ice/task.hxx>
#include <ice/task_sync_wait.hxx>

namespace ice
{

    static constexpr ice::ResourceAction_v2::Type Constant_RequiredResourceActionTypes[]{
        ice::ResourceAction_v2::Type::Load,
        ice::ResourceAction_v2::Type::Unload,
    };

    static constexpr ice::ResourceProviderAction_v2::Type Constant_RequiredProviderActionTypes[]{
        ice::ResourceProviderAction_v2::Type::Refresh,
    };

    struct ProviderActionCallback
    {
        ice::ResourceProviderAction_v2 const* action;

        auto operator()(ice::ResourceProvider_v2& provider) const noexcept -> ice::Task<ice::ResourceProviderAction_v2::Result>
        {
            if (action != nullptr)
            {
                co_return co_await action->handler(action->userdata, provider);
            }
            co_return ice::ResourceProviderAction_v2::Result::Skipped;
        }
    };

    struct TrackedProvider
    {
        ice::ResourceProvider_v2* provider;
        ice::ProviderActionCallback cb_provider_refresh;
        ice::ProviderActionCallback cb_provider_reset;

        //ice::Span<ice::ResourceProviderAction_v2 const> provider_actions;
        ice::Span<ice::ResourceAction_v2 const> resource_actions;
    };

    template<typename ActionTypeSpan, typename ActionSpan>
    bool check_for_required_types(
        ActionTypeSpan const& required_types,
        ActionSpan const& actions
    ) noexcept
    {
        bool has_all = true;
        for (auto type : required_types)
        {
            bool has_current = false;
            for (auto const& action : actions)
            {
                has_current |= action.type == type;
            }
            has_all &= has_current;
        }
        return has_all;
    }

    class ResourceTracker_Impl final : public ice::ResourceTracker_v2
    {
    public:
        ResourceTracker_Impl(ice::Allocator& alloc) noexcept
            : _allocator{ alloc }
            , _providers{ _allocator }
        {
        }

        ~ResourceTracker_Impl() noexcept = default;

        bool attach_provider(ice::ResourceProvider_v2* provider) noexcept override
        {
            ICE_ASSERT(provider != nullptr, "Trying to attach a nullptr as provider.");

            ice::u64 const hash_value = ice::hash_from_ptr(provider);

            bool has_provider = ice::pod::hash::has(_providers, hash_value);
            if (has_provider == false)
            {
                ice::Span<ice::ResourceProviderAction_v2 const> const provider_actions = provider->supported_provider_actions();

                bool const has_required_provider_actions = check_for_required_types(
                    ice::make_span(Constant_RequiredProviderActionTypes),
                    provider_actions
                );
                bool const has_required_actions = check_for_required_types(
                    ice::make_span(Constant_RequiredResourceActionTypes),
                    provider->supported_resource_actions()
                );

                if (has_required_provider_actions && has_required_actions)
                {
                    TrackedProvider tacked_provider{
                        .provider = provider,
                        .cb_provider_refresh = ice::addressof(provider_actions[0]),
                        .resource_actions = provider->supported_resource_actions()
                    };

                    if (provider_actions.size() > 1)
                    {
                        tacked_provider.cb_provider_reset.action = ice::addressof(provider_actions[1]);
                    }

                    ice::pod::hash::set(
                        _providers,
                        hash_value,
                        tacked_provider
                    );
                    has_provider = true;
                }
            }
            return has_provider;
        }

        bool detach_provider(ice::ResourceProvider_v2* provider) noexcept override
        {
            ice::pod::hash::remove(_providers, ice::hash_from_ptr(provider));
            return true;
        }

        void refresh_providers() noexcept override
        {
            for (auto const& entry : _providers)
            {
                ice::sync_wait(
                    [&]() noexcept -> ice::Task<>
                    {
                        co_await entry.value.cb_provider_refresh(*entry.value.provider);
                    }()
                );
            }
        }

        void reset_providers() noexcept override
        {
            for (auto const& entry : _providers)
            {
                entry.value.cb_provider_reset(*entry.value.provider);
            }
        }

        auto find_resource(
            ice::URI const& uri,
            ice::ResourceFlags_v2 flags = ice::ResourceFlags_v2::None
        ) const noexcept -> ice::Resource_v2 const* override
        {
            return nullptr;
        }


        auto create_resource(
            ice::URI const& uri,
            ice::Metadata const& metadata,
            ice::Data data
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto set_resource(
            ice::URI const& uri,
            ice::Resource_v2 const* resource,
            bool replace = false
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto load_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto release_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto unload_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto update_resource(
            ice::Resource_v2 const* resource,
            ice::Metadata const* metadata,
            ice::Data data
        ) noexcept -> ice::Task<ice::ResourceAction_v2::Result> override
        {
            co_return ice::ResourceAction_v2::Result{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::TrackedProvider> _providers;
    };

    auto create_resource_tracker(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker_v2>
    {
        return ice::make_unique<ice::ResourceTracker_v2, ice::ResourceTracker_Impl>(alloc, alloc);
    }

} // namespace ice
