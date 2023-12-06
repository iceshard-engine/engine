/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_packer_app.hxx"
#include <ice/os/windows.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>

void hscp_initialize_tags(ice::ParamList const& params) noexcept
{
    ice::log_tag_register(LogTag_Main);
    ice::log_tag_register(LogTag_Details);

    bool value = false;
    ice::params::find_first(params, Param_Verbose, value);
    ice::log_tag_enable(LogTag_Details.tag, value);
}

bool hscp_param_validate_file(ice::ParamList const&, ice::ParamInfo const&, ice::String value) noexcept
{
    return GetFileAttributesA(value._data) != INVALID_FILE_ATTRIBUTES;
}

static ice::UniquePtr<ice::ModuleRegister> global_modules;

HSCPackerApp::HSCPackerApp(ice::Allocator& alloc, ice::ParamList const& params)
    : _alloc{ alloc }
{
    global_modules = ice::create_default_module_register(_alloc);
    global_modules->load_module(_alloc, ice::load_log_module, ice::unload_log_module);
    hscp_initialize_tags(params);
}

HSCPackerApp::~HSCPackerApp()
{
    global_modules.reset();
}
