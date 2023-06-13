# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QmlCore
    URI "QtCore"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    PLUGIN_TARGET qtqmlcoreplugin
    CLASS_NAME QtQmlCorePlugin
    DEPENDENCIES
        QtQml/auto
    SOURCES
        qqmlsysteminformation_p.h
        qqmlsysteminformation.cpp
        qqmlstandardpaths_p.h
        qqmlstandardpaths.cpp
        qqmlcoreglobal_p.h
    DEFINES
        QT_BUILD_QML_CORE_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(QmlCore CONDITION QT_FEATURE_settings
    SOURCES
        qqmlsettings_p.h
        qqmlsettings.cpp
)

qt_internal_extend_target(QmlCore CONDITION QT_FEATURE_permissions
    SOURCES
        qqmlpermissions_p.h
)

qt_internal_add_docs(QmlCore
    doc/qtqmlcore.qdocconf
)
