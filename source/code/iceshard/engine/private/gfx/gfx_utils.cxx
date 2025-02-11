#include <ice/asset.hxx>
#include <ice/asset_storage.hxx>
#include <ice/config.hxx>
#include <ice/gfx/gfx_utils.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::gfx
{

    //auto await_shader_program_on(
    //    ice::String name,
    //    ice::AssetStorage& assets,
    //    ice::TaskScheduler& scheduler
    //) noexcept -> ice::TaskExpected<ice::render::PipelineProgramInfo>
    //{
    //    ice::Expected result = co_await await_shader_program(name, assets);
    //    co_await scheduler;
    //    co_return result;
    //}

    auto load_shader_program(
        ice::String name,
        ice::AssetStorage& assets
    ) noexcept -> ice::TaskExpected<ice::render::PipelineProgramInfo>
    {
        ice::Asset asset = assets.bind(ice::render::AssetCategory_Shader, name);
        ice::Data shader_data = co_await asset[AssetState::Runtime];
        if (shader_data.location == nullptr)
        {
            co_return E_Fail;
        }

        ice::render::PipelineProgramInfo result;

        ice::Data metadata;
        if (ice::Result mr = co_await asset.metadata(metadata); mr == ice::S_Ok)
        {
            ice::Config const meta = ice::config::from_data(metadata);
            ice::i32 shader_stage;
            if (ice::config::get(meta, "ice.shader.stage", shader_stage) == false)
            {
                co_return ice::E_Error;
            }

            if (ice::config::get(meta, "ice.shader.entry_point", result.entry_point) == false)
            {
                co_return ice::E_Error;
            }

            result.stage = static_cast<ice::render::ShaderStageFlags>(shader_stage);
        }
        else
        {
            co_return mr.error();
        }

        result.shader = *reinterpret_cast<ice::render::Shader const*>(shader_data.location);
        co_return result;
    }

}
