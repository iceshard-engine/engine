import Project from require 'ice.workspace.project'
import IceShard from require 'tools.iceshard'

with Project "IceShard"
    \application IceShard
    \script "ice.bat"
    \fastbuild_script "source/fbuild.bff"

    \set_conan_profile {
        windows: 'default'
        unix: 'clang-10.0-linux-x86_64'
    }

    \output "build"
    \sources "source/code"
    \working_dir "build"
    \finish!
