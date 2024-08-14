/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/build/config.hxx>

namespace ice::build
{

    //! \brief The seed used to generate hash values using the ice::hash32 function.
    static constexpr uint32_t Constant_Hash32_DefaultSeed = 0x428639DA;

    //! \brief The seed used to generate hash values using the ice::hash function.
    static constexpr uint32_t Constant_Hash64_DefaultSeed = 0x8642DA39;

    //! \brief The seed used to generate hash values for ice::StringID.
    static constexpr uint32_t Constant_StringID_DefaultSeed = 0xDA864239;

    //! \brief Switch controling the default implementation used for ice::StringID.
    //!
    //! \note Currently StringID will contain debug fields when compiled on 'Debug' or 'Develop' builds.
    static constexpr bool Constant_StringID_DebugInfoEnabled = current_config == Configuration::Debug || current_config == Configuration::Develop;

    //! \brief The seed used to generate hash values for ice::detail::ShardName.
    static constexpr uint32_t Constant_ShardName_DefaultSeed = 0x77a23ab1;

    //! \brief The seed used to generate hash values for ice::detail::PayloadID.
    static constexpr uint32_t Constant_ShardPayloadID_DefaultSeed = 0x3ab177a2;

} // namespace ice::build
