import Path, File from require "ice.core.fs"
import Json from require "ice.util.json"

input_file = arg[1]
output_file = arg[2]

shader_types = {
    vert:'vert'
    frag:'frag'
    vtx:'vert'
    pix:'frag'
}

shader_type = input_file\match "([^%-]+)%.spv"
shader_meta = {
    resource: { meta: { type: 0, version: 0 } },
    shader: { target: 1, stage: shader_type == 'vert' and 1 or 2 }
}

if f = File\open output_file, mode:'w+'
    f\write Json\encode shader_meta
    f\close!
