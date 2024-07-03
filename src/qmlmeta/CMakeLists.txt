# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(module_dynamic_qml_imports
    QtQml.Models/auto
)

if (QT_FEATURE_qml_worker_script)
    list(APPEND module_dynamic_qml_imports
        QtQml.WorkerScript/auto
    )
endif()

qt_internal_add_qml_module(QmlMeta
    URI "QtQml"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    __QT_INTERNAL_SYSTEM_MODULE
    PLUGIN_TARGET qmlplugin
    CLASS_NAME QtQmlPlugin
    PAST_MAJOR_VERSIONS 2
    IMPORTS
        QML/1.0
        ${module_dynamic_qml_imports}
    SOURCES
        types/qqmlbind.cpp types/qqmlbind_p.h
        types/qqmlconnections.cpp types/qqmlconnections_p.h
        types/qqmlloggingcategory.cpp types/qqmlloggingcategory_p.h
    LIBRARIES
        Qt::QmlPrivate
        Qt::QmlModelsPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::QmlModels
    PRIVATE_MODULE_INTERFACE
        Qt::QmlPrivate
        Qt::QmlModelsPrivate
)

add_dependencies(QmlMeta BuiltinsOutput)

# Linking to the static qml plugin should also automatically link to the worker script
# static plugin otherwise you get errors like
#  module "QtQml.WorkerScript" plugin "workerscriptplugin" not found
#     import QtQuick 2.0
#     ^
if(QT_FEATURE_qml_worker_script)
    qt_internal_extend_target(QmlMeta
        LIBRARIES
            Qt::QmlWorkerScript
    )
    _qt_internal_add_qml_static_plugin_dependency(qmlplugin workerscriptplugin)
endif()

# Same for the QmlModels qml plugin, otherwise you get
#  module "QtQuick" version 6.6 cannot be imported because:
#  module "QtQml.Models" plugin "modelsplugin" not found
#     import QtQuick
#     ^
_qt_internal_add_qml_static_plugin_dependency(qmlplugin modelsplugin)

qt_internal_extend_target(QmlMeta CONDITION QT_FEATURE_qml_animation
    SOURCES
        types/qqmltimer.cpp types/qqmltimer_p.h
    INCLUDE_DIRECTORIES
        ../qml/animations
)

qt_internal_extend_target(QmlMeta CONDITION QT_FEATURE_qml_locale
    SOURCES
        types/qqmllocaleenums_p.h
)

# The Qml_sync_headers target doesn't exist when this CMakeLists is executed. So the qmlplugin
# plugin target misses the dependency on Qml_sync_headers target. This leads to the
# "unknown IID" issue when moc processes plugin sources, because of missing header aliases.
# Qml_sync_headers target is created later by the finalizer in the directory scope so we add this
# dependency manually instead of relying on qt_internal_add_qml_module logic. Same is applicable
# for the QmlMeta target.
set_property(TARGET qmlplugin APPEND PROPERTY AUTOGEN_TARGET_DEPENDS Qml_sync_headers)
set_property(TARGET QmlMeta APPEND PROPERTY AUTOGEN_TARGET_DEPENDS Qml_sync_headers)
