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

                ice::pod::hash::set(
                    _providers,
                    hash_value,
                    provider
                );
                has_provider = true;
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
                        co_await entry.value->refresh();
                    }()
                );
            }
        }

        void reset_providers() noexcept override
        {
            for (auto const& entry : _providers)
            {
                ice::sync_wait(
                    [&]() noexcept -> ice::Task<>
                    {
                        co_await entry.value->reset();
                    }()
                );
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
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto set_resource(
            ice::URI const& uri,
            ice::Resource_v2 const* resource,
            bool replace = false
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto load_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto release_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto unload_resource(
            ice::Resource_v2 const* resource
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

        auto update_resource(
            ice::Resource_v2 const* resource,
            ice::Metadata const* metadata,
            ice::Data data
        ) noexcept -> ice::Task<ice::ResourceActionResult> override
        {
            co_return ice::ResourceActionResult{
                .resource_status = ice::ResourceStatus_v2::Failure
            };
        }

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::ResourceProvider_v2*> _providers;
    };

    auto create_resource_tracker(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::ResourceTracker_v2>
    {
        return ice::make_unique<ice::ResourceTracker_v2, ice::ResourceTracker_Impl>(alloc, alloc);
    }

} // namespace ice
