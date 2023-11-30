#include "hsc_reader_app.hxx"
#include <ice/os/windows.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>
#include <tuple>

auto hscr_flag_param(ice::ParamInfo const& info) noexcept
{
    return ice::ParamDefinition<bool>{ .name = info.name, .name_short = info.name_short, .flags = ice::ParamFlags::IsFlag };
}

void hscr_initialize_tags(ice::ParamList const& params) noexcept
{
    static const std::tuple<ice::LogTagDefinition const&, ice::ParamInfo> tags[]{
        { LogTag_Main, Param_InfoHeader },
        { LogTag_InfoChunks, Param_InfoChunks },
        { LogTag_InfoResources, Param_InfoResources },
        { LogTag_InfoPaths, Param_InfoResourcePaths },
    };

    bool tag_enable = false;
    for (auto const& tag : tags)
    {
        ice::log_tag_register(std::get<0>(tag));

        // Check if the tag can be enabled.
        ice::ParamDefinition<bool> temp_param = hscr_flag_param(std::get<1>(tag));
        ice::params::find_first(params, temp_param, tag_enable);
        ice::log_tag_enable(std::get<0>(tag).tag, std::exchange(tag_enable, false));
    }
}

bool hscr_param_validate_file(ice::ParamList const&, ice::ParamInfo const&, ice::String value) noexcept
{
    return GetFileAttributesA(value._data) != INVALID_FILE_ATTRIBUTES;
}

static ice::UniquePtr<ice::ModuleRegister> global_modules;

HSCReaderApp::HSCReaderApp(ice::Allocator& alloc, ice::ParamList const& params)
    : _alloc{ alloc }
{
    global_modules = ice::create_default_module_register(_alloc);
    global_modules->load_module(_alloc, ice::load_log_module, ice::unload_log_module);

    hscr_initialize_tags(params);
}

HSCReaderApp::~HSCReaderApp()
{
    global_modules.reset();
}
