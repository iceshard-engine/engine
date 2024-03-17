/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_app.hxx"
#include <ice/log_module.hxx>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>

struct WebAsmLog : ice::Module<ice::LogModule>
{
    IS_WORKAROUND_MODULE_INITIALIZATION(WebAsmLog);
};

void wasm_app_main(void* userdata) {
    ice::platform::webasm::WebAsmApp* app = reinterpret_cast<ice::platform::webasm::WebAsmApp*>(userdata);
    app->main_update();
}

int main() {
    static ice::platform::webasm::WebAsmApp app{};
    emscripten_set_main_loop_arg(wasm_app_main, &app, 0, true);
    return 0;
}
