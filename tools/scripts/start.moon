import Path, Dir, File from require "ice.core.fs"
import Log from require "ice.core.logger"

config = arg[1] or 'Develop'
arch = arg[2] or 'x64'
app = arg[3] or 'test'

build_dir = Path\normalize Path\join Dir\current!, '../../build/bin'
unless Dir\exists build_dir then error "Binaries directory '#{build_dir}' does not exist"

pipeline_dir = nil
for name in Dir\list build_dir
    if name\match arch
        pipeline_dir = Path\join build_dir, name
        break

unless pipeline_dir then error "Failed to find matching pipeline for arch = #{arch}"

config_dir = nil
for name in Dir\list pipeline_dir
    if name\match config
        config_dir = Path\join pipeline_dir, name
        break

unless config_dir then error "Failed to find matching target for config = #{config}"
app_path = Path\join config_dir, app, app .. os.osselect win:'.exe', unix:''

unless File\exists app_path then error "Failed to find executable: #{app_path}"

Dir\enter '../../build', ->
    Log\info "Executing app: #{app_path}"
    os.execute app_path
