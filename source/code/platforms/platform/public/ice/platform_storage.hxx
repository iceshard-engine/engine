/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform.hxx>
#include <ice/string/string.hxx>
#include <ice/shard.hxx>

namespace ice::platform
{

    //! \brief Name of the application used to create sub directories in selected paths.
    //! \note This is optional, but allows to easily get paths that are already "unique" to your application.
    static constexpr ice::ShardID Shard_StorageAppName = "platform/storage/app_name`char const*"_shardid;

    //! \brief Provides access to various storage locations categorized for specific uses.
    //! \note All paths returned end with a backslash.
    struct StoragePaths
    {
        //! \brief This value should be used to create resource providers.
        //! \return One or multiple locations containing the applications data.
        virtual auto data_locations() const noexcept -> ice::Span<ice::String const> = 0;

        //! \return Location for storing save data between game sessions.
        //! \note This location may be synced with the cloud on some platforms.
        virtual auto save_location() const noexcept -> ice::String = 0;

        //! \return Location for persistent generated data between game sessions.
        //! \note May be cleared by the user manually.
        virtual auto cache_location() const noexcept -> ice::String = 0;

        //! \return Location for temporary generated data.
        //! \note Data stored here may be deleted by the system at any time.
        virtual auto temp_location() const noexcept -> ice::String { return cache_location(); }

        enum class UserContentType : uint8_t
        {
            //! \brief Location for pictures / images created by the user.
            Screenshot = 0x01,

            //! \brief Location for saving custom data files, like character templates to be shared.
            CustomData = 0x02,
        };

        //! \return Location for storing user created data (ex.: screenshots).
        //! \note If a platform does not specify a custom location, any user-accessible location should be used.
        virtual auto usercontent_location(UserContentType content_type) const noexcept -> ice::String
        {
            return save_location();
        }

        //! \brief This value should be used to access shared libraries to be loaded.
        //! \note This location is unaffected by the AppName and is system dependent.
        //! \return Single location with additional shared libraries to be loaded.
        virtual auto dylibs_location() const noexcept -> ice::String = 0;
    };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::StoragePaths> = FeatureFlags::StoragePaths;

} // namespace ice::platform
