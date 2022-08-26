# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickShapesPrivate Module:
#####################################################################

qt_internal_add_qml_module(QuickShapesPrivate
    URI "QtQuick.Shapes"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qmlshapesplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    CLASS_NAME QmlShapesPlugin
    DEPENDENCIES
        QtQuick/auto
    INTERNAL_MODULE
    SOURCES
        qquickshape.cpp qquickshape_p.h
        qquickshape_p_p.h
        qquickshapegenericrenderer.cpp qquickshapegenericrenderer_p.h
        qquickshapesglobal.h qquickshapesglobal_p.h
        qquickshapesoftwarerenderer.cpp qquickshapesoftwarerenderer_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

# We need to do additional initialization, so we have to provide our own
# plugin class rather than using the generated one
qt_internal_extend_target(qmlshapesplugin
    SOURCES   qquickshapesplugin.cpp
    LIBRARIES Qt::QuickShapesPrivate
)

# Resources:
set(qtquickshapes_resource_files
    "shaders_ng/conicalgradient.frag.qsb"
    "shaders_ng/conicalgradient.vert.qsb"
    "shaders_ng/lineargradient.frag.qsb"
    "shaders_ng/lineargradient.vert.qsb"
    "shaders_ng/radialgradient.frag.qsb"
    "shaders_ng/radialgradient.vert.qsb"
)

qt_internal_add_resource(QuickShapesPrivate "qtquickshapes"
    PREFIX
        "/qt-project.org/shapes"
    FILES
        ${qtquickshapes_resource_files}
)

