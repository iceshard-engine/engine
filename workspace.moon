import Project from require 'ice.workspace.project'
import IceShard from require 'tools.iceshard'

with Project "IceShard"
    \application IceShard
    \script "ice.bat"
    \fastbuild_script "source/fbuild.bff"

    \output "build"
    \sources "source/code"
    \working_dir "build"
    \finish!
