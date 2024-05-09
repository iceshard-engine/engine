/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool_app.hxx>

class AssetCompilerApp : public ice::tool::ToolApp<AssetCompilerApp>
{
public:
    AssetCompilerApp() noexcept
        : _inputs{ _allocator }
    {
    }

    bool setup(ice::Params& params) noexcept override
    {
        ice::params_define(params, {
                .name = "input",
                .description = "Input files required to create an asset",
                .min = 1,
                .flags = ice::ParamFlags::IsRequired,
            },
            _inputs
        );
        return true;
    }

    auto run() noexcept -> ice::i32 override
    {
        return 0;
    }

public: // Tool information
    auto name() const noexcept -> ice::String override { return "asset_compiler"; }
    auto version() const noexcept -> ice::String override { return "0.1.0"; }
    auto description() const noexcept -> ice::String override
    {
        return "Compiles input files into a single asset resources. This resource is optimized for loading using IceShard.";
    }

private:
    ice::Array<ice::String> _inputs;
};
