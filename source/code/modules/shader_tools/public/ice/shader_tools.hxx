/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>

namespace ice
{

    //! \brief The rendering platform for which to build shaders.
    enum class ShaderTargetPlatform : ice::u8
    {
        //! \brief Generates shaders for Vulkan renderer (.glsl or .spv)
        GLSL,
        //! \brief Generates shaders for DX11/DX12 renderer (.hsls)
        //! \note Not yet implemented.
        HLSL,
        //! \brief Generates shaders for WebGPU renderer (.hsls)
        WGSL,
    };

    //! \brief The stage to which the shader should be built.
    enum class ShaderStage : ice::u8
    {
        //! \brief Returns the complete generated shader code from .asl files.
        Transpiled,
        //! \brief Returns precompiled bytecode if the platform supports it.
        //! \note Data returned from this stage can be directly passed to the target renderer.
        Compiled
    };

    //! \brief Parameter name used for selecting the renderer platform.
    static constexpr ice::ShardID ShardID_ShaderTargetPlatform = "shader:target"_shardid;

    //! \brief Parameter name used for selecting shader stage.
    static constexpr ice::ShardID ShardID_ShaderStage = "shader:stage"_shardid;

} // namespace ice

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::ShaderTargetPlatform> = ice::shard_payloadid("ice::ShaderPlatformTarget");

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::ShaderStage> = ice::shard_payloadid("ice::ShaderStage");
