#pragma once
#include <coroutine>
#include <ice/data.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    struct AssetHandle;

    enum class AssetState : ice::u32
    {
        //! \brief The asset could not be accessed.
        //! \detail Either an error occured or the asset does not exist.
        Invalid,

        //! \brief The asset data is represented in raw format of the source file.
        //! \detail This means that you might get raw jpg or png image data.
        Raw,

        //! \brief The asset data is baked into a specific engine representation.
        //! \detail Please take a look at the data representation for the asset you are trying to access.
        Baked,

        //! \brief The asset is loaded and pointers are patched using a specific engine representation.
        //! \detail Please take a look at the data representation for the asset you are trying to access.
        Loaded,

        //! \brief The asset is loaded into a system and only contains a runtime handle.
        //! \detail This is particullary useful if you want to have a single system to manage specific assets, for example a texture loader.
        Runtime,
    };

    struct Asset
    {
        ice::AssetHandle* handle;
        ice::AssetState state;
        ice::Data data;
    };

} // namespace ice
