# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickParticles Module:
#####################################################################

qt_internal_add_qml_module(QuickParticlesPrivate
    URI "QtQuick.Particles"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET particlesplugin
    CLASS_NAME QtQuick2ParticlesPlugin
    DEPENDENCIES
        QtQuick/auto
    INTERNAL_MODULE
    SOURCES
        qquickage.cpp qquickage_p.h
        qquickangledirection.cpp qquickangledirection_p.h
        qquickcumulativedirection.cpp qquickcumulativedirection_p.h
        qquickcustomaffector.cpp qquickcustomaffector_p.h
        qquickdirection.cpp qquickdirection_p.h
        qquickellipseextruder.cpp qquickellipseextruder_p.h
        qquickfriction.cpp qquickfriction_p.h
        qquickgravity.cpp qquickgravity_p.h
        qquickgroupgoal.cpp qquickgroupgoal_p.h
        qquickimageparticle.cpp qquickimageparticle_p.h
        qquickitemparticle.cpp qquickitemparticle_p.h
        qquicklineextruder.cpp qquicklineextruder_p.h
        qquickmaskextruder.cpp qquickmaskextruder_p.h
        qquickparticleaffector.cpp qquickparticleaffector_p.h
        qquickparticleemitter.cpp qquickparticleemitter_p.h
        qquickparticleextruder.cpp qquickparticleextruder_p.h
        qquickparticlegroup.cpp qquickparticlegroup_p.h
        qquickparticlepainter.cpp qquickparticlepainter_p.h
        qquickparticlesystem.cpp qquickparticlesystem_p.h
        qquickpointattractor.cpp qquickpointattractor_p.h
        qquickpointdirection.cpp qquickpointdirection_p.h
        qquickrectangleextruder.cpp qquickrectangleextruder_p.h
        qquickspritegoal.cpp qquickspritegoal_p.h
        qquicktargetdirection.cpp qquicktargetdirection_p.h
        qquicktrailemitter.cpp qquicktrailemitter_p.h
        qquickturbulence.cpp qquickturbulence_p.h
        qquickv4particledata.cpp qquickv4particledata_p.h
        qquickwander.cpp qquickwander_p.h
        qtquickparticlesglobal_p.h
    NO_PCH_SOURCES
        # these undef QT_NO_FOREACH
        qquickcumulativedirection.cpp
        qquickimageparticle.cpp
        qquickitemparticle.cpp
        qquickparticleaffector.cpp
        qquickparticleemitter.cpp
        qquickparticlegroup.cpp
        qquickparticlepainter.cpp
        qquicktrailemitter.cpp
        qquickturbulence.cpp
        # end undef QT_NO_FOREACH
    DEFINES
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    )

# Resources:
set(particles_resource_files
    "particleresources/fuzzydot.png"
    "particleresources/glowdot.png"
    "particleresources/noise.png"
    "particleresources/star.png"
)

qt_internal_add_resource(QuickParticlesPrivate "particles"
    PREFIX
        "/"
    FILES
        ${particles_resource_files}
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders1"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_simplepoint.vert.qsb"
        "shaders_ng/imageparticle_simplepoint.frag.qsb"
    DEFINES
        POINT
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders2"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_coloredpoint.vert.qsb"
        "shaders_ng/imageparticle_coloredpoint.frag.qsb"
    DEFINES
        POINT
        COLOR
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders3"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_colored.vert.qsb"
        "shaders_ng/imageparticle_colored.frag.qsb"
    DEFINES
        COLOR
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders4"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_deformed.vert.qsb"
        "shaders_ng/imageparticle_deformed.frag.qsb"
    DEFINES
        DEFORM
        COLOR
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders5"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_tabled.vert.qsb"
        "shaders_ng/imageparticle_tabled.frag.qsb"
    DEFINES
        TABLE
        DEFORM
        COLOR
)

qt_internal_add_shaders(QuickParticlesPrivate "particles_shaders6"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    GLSL
        "150,120,100es,300es"
    PREFIX
        "/particles"
    FILES
        "shaders_ng/imageparticle.vert"
        "shaders_ng/imageparticle.frag"
    OUTPUTS
        "shaders_ng/imageparticle_sprite.vert.qsb"
        "shaders_ng/imageparticle_sprite.frag.qsb"
    DEFINES
        SPRITE
        TABLE
        DEFORM
        COLOR
    ZORDER_LOC
        8
)

qt_internal_extend_target(QuickParticlesPrivate CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

#### Keys ignored in scope 3:.:.:particles.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

qt_internal_extend_target(QuickParticlesPrivate CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)
