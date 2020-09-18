import Project from require 'ice.workspace.project'

with Project "IceShard"
    \script "ice.bat"
    \fastbuild_script "source/fbuild.bff"

    \output "build"
    \sources "source/code"
    \working_dir "build"
    \finish!
