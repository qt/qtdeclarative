# Generated from particles.pro.

#####################################################################
## QuickParticles Module:
#####################################################################

qt_internal_add_module(QuickParticles
    INTERNAL_MODULE
    GENERATE_METATYPES
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
        qquickparticleflatset_p.h
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

qt_internal_add_resource(QuickParticles "particles"
    PREFIX
        "/"
    FILES
        ${particles_resource_files}
)
set(particles1_resource_files
    "shaders_ng/imageparticle_colored.frag.qsb"
    "shaders_ng/imageparticle_colored.vert.qsb"
    "shaders_ng/imageparticle_deformed.frag.qsb"
    "shaders_ng/imageparticle_deformed.vert.qsb"
    "shaders_ng/imageparticle_simple.frag.qsb"
    "shaders_ng/imageparticle_simple.vert.qsb"
    "shaders_ng/imageparticle_sprite.frag.qsb"
    "shaders_ng/imageparticle_sprite.vert.qsb"
    "shaders_ng/imageparticle_tabled.frag.qsb"
    "shaders_ng/imageparticle_tabled.vert.qsb"
)

qt_internal_add_resource(QuickParticles "particles1"
    PREFIX
        "/particles"
    FILES
        ${particles1_resource_files}
)


#### Keys ignored in scope 1:.:.:particles.pro:<TRUE>:
# MODULE = "quickparticles"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick/Particles"
# QML_IMPORT_NAME = "QtQuick.Particles"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

qt_internal_extend_target(QuickParticles CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

#### Keys ignored in scope 3:.:.:particles.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

qt_internal_extend_target(QuickParticles CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)

set_target_properties(QuickParticles PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Particles
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Particles"
)

qt6_qml_type_registration(QuickParticles)
