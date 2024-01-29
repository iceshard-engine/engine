/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform_storage.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/span.hxx>

#if ISP_WINDOWS

namespace ice::platform::win32
{

    class Win32Storage : public ice::platform::StoragePaths
    {
    public:
        Win32Storage(ice::Allocator& alloc, ice::Span<ice::Shard const> params) noexcept;

        //! \brief Requests the storage to alter most paths by appeding the given string at the end of them.
        //! \note The storage implementation is not required to alter all paths.
        //!   Additionally the 'data_location()' path will never be altered by this operation.
        //!
        //! \param[in] name The name to be appended to all paths.
        //! \return 'true' If the operation was supported and successful.
        bool set_appname(ice::String name) noexcept;

        auto data_locations() const noexcept -> ice::Span<ice::String const> override;
        auto save_location() const noexcept -> ice::String override { return _save_location; }
        auto cache_location() const noexcept -> ice::String override { return _cache_location; }

        auto temp_location() const noexcept -> ice::String override { return _temp_location; }
        auto usercontent_location(UserContentType content_type) const noexcept -> ice::String override { return {}; }

        auto dylibs_location() const noexcept -> ice::String override;

        [[deprecated]]
        auto internal_data() const noexcept -> ice::String { return {}; }
        [[deprecated]]
        auto external_data() const noexcept -> ice::String { return {}; }
        [[deprecated]]
        auto save_data() const noexcept -> ice::String { return {}; }

    protected:
        void reload_paths(ice::String appname) noexcept;

    private:
        ice::HeapString<> _save_location;
        ice::HeapString<> _cache_location;
        ice::HeapString<> _temp_location;
        ice::HeapString<> _pictures_location;
        ice::HeapString<> _other_location;
    };


} // namespace ice

#endif
