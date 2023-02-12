import Path, Dir, File from require 'ice.core.fs'
import Log from require 'ice.core.logger'
import Json from require 'ice.util.json'

build_shader = (vkinfo, source, shader_type) ->
    out_filename = source\gsub "%.glsl", ".spv"
    out_metafile = out_filename .. '.isrm'

    if os.execute "\"#{vkinfo.glslc}\" --target-env=vulkan -fshader-stage=#{shader_type} -o #{out_filename} #{source}"
        Log\info "Compilation success: #{source} -> #{out_filename}"
        shader_meta = {
            resource: { meta: { type: 0, version: 0 } },
            shader: { target: 1, stage: shader_type == 'vert' and 1 or 2 }
        }

        if f = File\open out_metafile, mode:'w+'
            f\write Json\encode shader_meta
            f\close!

build_shaders = ->
    -- TODO: Update once the working dir is fixed to be the project root directory.
    Dir\enter '../../source/data'

    vkinfo = { }

    -- Get access to Vulkan SDK
    vkinfo.path = os.getenv 'VULKAN_SDK'
    unless vkinfo.path
        Log\error "Cannot compile shaders without access to the Vulkan SDK."
        return

    Log\info "- Vulkan SDK: #{vkinfo.path}"

    vkinfo.glslc = Path\join vkinfo.path, 'Bin/glslc.exe'
    Log\info "- GLSL Compiler: #{vkinfo.glslc}"

    shader_types = {
        vert:'vert'
        frag:'frag'
        vtx:'vert'
        pix:'frag'
    }

    for glsl_file in *Dir\find_files 'shaders', recursive:true, filter: (file) -> (Path\extension file) == '.glsl'
        shader_type = glsl_file\match "([^%-]+)%.glsl"
        build_shader vkinfo, glsl_file, shader_types[shader_type] if shader_types[shader_type]

build_shaders!
