/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/build/build.hxx>

// TODO: Move to a different file or remove entirely
namespace ice::config
{

    //! \brief The seed used to generate hash values using the ice::hash32 function.
    static constexpr ice::u32 Hash32_DefaultSeed = 0x428639DA;

    //! \brief The seed used to generate hash values using the ice::hash function.
    static constexpr ice::u32 Hash64_DefaultSeed = 0x8642DA39;

    //! \brief The seed used to generate hash values for ice::StringID.
    static constexpr ice::u32 StringID_DefaultSeed = 0xDA864239;

    //! \brief Switch controling the default implementation used for ice::StringID.
    //!
    //! \note Currently StringID will contain debug fields when compiled on 'Debug' or 'Develop' builds.
    static constexpr bool StringID_DebugInfoEnabled = ice::build::is_debug || ice::build::is_develop;

    //! \brief The seed used to generate hash values for ice::detail::ShardName.
    static constexpr ice::u32 ShardName_DefaultSeed = 0x77a23ab1;

    //! \brief The seed used to generate hash values for ice::detail::PayloadID.
    static constexpr ice::u32 ShardPayloadID_DefaultSeed = 0x3ab177a2;

} // namespace ice::config
