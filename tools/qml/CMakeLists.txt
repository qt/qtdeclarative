# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qml.pro.

#####################################################################
## qml App:
#####################################################################

qt_internal_add_app(qml_tool
    TARGET_DESCRIPTION "QML Runtime"
    SOURCES
        conf.h
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

set_target_properties(qml_tool PROPERTIES WIN32_EXECUTABLE FALSE)
set_target_properties(qml_tool PROPERTIES OUTPUT_NAME "qml")

# Turn the tool into its own self-contained qml module
qt6_add_qml_module(qml_tool
    RESOURCE_PREFIX "/qt-project.org/imports"
    URI QmlRuntime.Config
    VERSION 1.0
    QML_FILES
        default.qml
        resizeToItem.qml
        ResizeItemToWindow.qml
        ResizeWindowToItem.qml
    RESOURCES
        resources/qml-64.png
)

qt_internal_extend_target(qml_tool CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)

qt_internal_extend_target(qml_tool CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(qml_tool CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
# Very hacky fix to ensure qml_tool_autogen doesn't lose dependencies
# on Qml_sync_headers.
# See QTBUG-133725
get_target_property(the_libs qml_tool LINK_LIBRARIES)
list(REMOVE_DUPLICATES the_libs)
set_target_properties(qml_tool PROPERTIES LINK_LIBRARIES "${the_libs}")

# Install the entitlements file next to the binary, so that the post-CI signing process uses it
# to ensure 3rd party qml plugins can also be loaded.
if(MACOS)
    qt_install(FILES qml.entitlements
        DESTINATION "${INSTALL_BINDIR}"
    )
endif()
