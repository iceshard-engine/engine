#include <ice/resource_provider.hxx>
#include <ice/resource_provider_action.hxx>
#include <ice/resource_action.hxx>
#include <ice/os/windows.hxx>
#include <ice/heap_string.hxx>

#include "resorce_file_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class ResourceProvider_Win32Filesystem final : public ice::ResourceProvider_v2
    {
    public:
        ResourceProvider_Win32Filesystem(
            ice::Allocator& alloc,
            ice::Utf8String path
        ) noexcept
            : _allocator{ alloc }
            , _path{ path }
        { }

        auto supported_provider_actions_count() const noexcept -> ice::u32 override
        {
            return 1;
        }

        auto supported_provider_actions() const noexcept -> ice::Span<ice::ResourceProviderAction_v2 const> override;

        auto supported_resource_actions_count() const noexcept -> ice::u32
        {
            return 2;
        }

        auto supported_resource_actions() const noexcept -> ice::Span<ice::ResourceAction_v2 const> override;

        bool query_changes(ice::pod::Array<ice::Resource_v2 const*>& out_changes) const noexcept
        {
            return false;
        }

        void refresh() noexcept
        {

            ice::HeapString<wchar_t> u16str = ice::utf8_to_wide(_allocator, _path);
            ice::HeapString<wchar_t> u16str2 = u16str;

            ice::string::push_back(u16str, L".json");
            ice::string::push_back(u16str2, L".isrm");

            create_resource_from_loose_files(_allocator, u16str2, u16str);

            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "Hello, World!");
        }

    private:
        ice::Allocator& _allocator;
        ice::Utf8String _path;
    };

    auto refresh_resource_provider(
        ice::Userdata,
        ice::ResourceProvider_v2& provider
    ) noexcept -> ice::Task<ice::ResourceProviderAction_v2::Result>
    {
        using ActionResult = ice::ResourceProviderAction_v2::Result;

        static_cast<ResourceProvider_Win32Filesystem&>(provider).refresh();

        co_return ActionResult::Skipped;
    }

    auto ResourceProvider_Win32Filesystem::supported_provider_actions() const noexcept -> ice::Span<ice::ResourceProviderAction_v2 const>
    {
        using ActionType = ice::ResourceProviderAction_v2::Type;
        using ActionHandler = ice::ResourceProviderAction_v2::Handler;

        static const ice::ResourceProviderAction_v2 actions[]{
            ice::ResourceProviderAction_v2{
                .type = ActionType::Refresh,
                .handler = refresh_resource_provider,
                .userdata = nullptr
            }
        };

        return actions;
    }

    auto ResourceProvider_Win32Filesystem::supported_resource_actions() const noexcept -> ice::Span<ice::ResourceAction_v2 const>
    {
        using ActionType = ice::ResourceAction_v2::Type;
        using ActionHandler = ice::ResourceAction_v2::Handler;

        static const ice::ResourceAction_v2 actions[]{
            ice::ResourceAction_v2{
                .type = ActionType::Load,
                .handler = nullptr,
                .userdata = nullptr
            },
            ice::ResourceAction_v2{
                .type = ActionType::Unload,
                .handler = nullptr,
                .userdata = nullptr
            }
        };

        return actions;
    }

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>
    {
        return ice::make_unique<ice::ResourceProvider_v2, ice::ResourceProvider_Win32Filesystem>(alloc, alloc, path);
    }

} // namespace ice

#endif // #if ISP_WINDOWS
