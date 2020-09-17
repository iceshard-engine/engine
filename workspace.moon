import Project from require 'ice.workspace.project'

with Project "IceShard"
    \fastbuild_script "source/fbuild.bff"
    \fastbuild_vstudio_solution "test"

    \output "build"
    \sources "source/code"
    \working_dir "build"
    \finish!
