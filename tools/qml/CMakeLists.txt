# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qml.pro.

#####################################################################
## qml App:
#####################################################################

qt_internal_add_app(qml
    TARGET_DESCRIPTION "QML Runtime"
    SOURCES
        conf.h
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

set_target_properties(qml PROPERTIES WIN32_EXECUTABLE FALSE)

set_source_files_properties(
    conf/default.qml
    conf/resizeToItem.qml
    conf/content/resizeItemToWindow.qml
    conf/content/resizeWindowToItem.qml
    PROPERTIES QT_SKIP_QUICKCOMPILER 1
)

# Resources:
set(qml_resource_files
    "conf/content/resizeItemToWindow.qml"
    "conf/content/resizeWindowToItem.qml"
    "conf/default.qml"
    "conf/resizeToItem.qml"
    "resources/qml-64.png"
)

qt_internal_add_resource(qml qml
    PREFIX
        "/qt-project.org/QmlRuntime"
    FILES
        ${qml_resource_files}
)

# Turn the tool into its own self-contained qml module
qt6_add_qml_module(qml
    RESOURCE_PREFIX "/qt-project.org"
    URI QmlRuntime.Config
    VERSION 1.0
)

qt_internal_extend_target(qml CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)

qt_internal_extend_target(qml CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 4:.:.:qml.pro:WIN32:
# RC_ICONS = "resources/qml.ico"

#### Keys ignored in scope 5:.:.:qml.pro:APPLE:
# ICON = "resources/qml.icns"
# OTHER_FILES = "resources/Info.plist"
# QMAKE_INFO_PLIST = "resources/Info.plist"

qt_internal_extend_target(qml CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
