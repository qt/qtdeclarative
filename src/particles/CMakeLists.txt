# Generated from particles.pro.

#####################################################################
## QuickParticles Module:
#####################################################################

add_qt_module(QuickParticles
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
        qquickparticlesmodule.cpp qquickparticlesmodule_p.h
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
    DEFINES
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(particles_resource_files
    "particleresources/fuzzydot.png"
    "particleresources/glowdot.png"
    "particleresources/noise.png"
    "particleresources/star.png"
)

add_qt_resource(QuickParticles "particles"
    PREFIX
        "/"
    FILES
        ${particles_resource_files}
)
set(particles1_resource_files
    "shaders/customparticle.frag"
    "shaders/customparticle.vert"
    "shaders/customparticle_core.frag"
    "shaders/customparticle_core.vert"
    "shaders/customparticletemplate.vert"
    "shaders/customparticletemplate_core.vert"
    "shaders/imageparticle.frag"
    "shaders/imageparticle.vert"
    "shaders/imageparticle_core.frag"
    "shaders/imageparticle_core.vert"
)

add_qt_resource(QuickParticles "particles1"
    PREFIX
        "/particles"
    FILES
        ${particles1_resource_files}
)


#### Keys ignored in scope 1:.:.:particles.pro:<TRUE>:
# MODULE = "quickparticles"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(QuickParticles CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

#### Keys ignored in scope 3:.:.:particles.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

extend_target(QuickParticles CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)

extend_target(QuickParticles CONDITION QT_FEATURE_quick_shadereffect
    SOURCES
        qquickcustomparticle.cpp qquickcustomparticle_p.h
)
