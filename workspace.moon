import Project from require 'ice.workspace.project'
import IceShard from require 'tools.iceshard'

with Project "IceShard"
    \application IceShard

    -- Set explicitly all the additional compilers we have defined
    \set 'project.fbuild.user_includes', {
        'source/asset_compiler.bff'
        'source/isrm_compiler.bff'
    }

    \finish!
