import Path, Dir, File from require "ice.core.fs"
import Log from require "ice.core.logger"

config = arg[1] or 'Develop'
arch = arg[2] or 'x64'
app = arg[3] or 'test'

Dir\enter '../../build', ->
    test = Path\join "bin/#{arch}/Windows-#{config}-msvc-#{arch}-v143/#{app}/#{app}.exe"
    Log\info "Trying to run app: #{test}"
    os.execute test
