# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickControls2 Module:
#####################################################################

qt_internal_add_qml_module(QuickControls2
    URI "QtQuick.Controls"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    DESIGNER_SUPPORTED
    CLASS_NAME QtQuickControls2Plugin
    PLUGIN_TARGET qtquickcontrols2plugin
    OPTIONAL_IMPORTS
        QtQuick.Controls.Fusion/auto
        QtQuick.Controls.Material/auto
        QtQuick.Controls.Imagine/auto
        QtQuick.Controls.Universal/auto
        QtQuick.Controls.FluentWinUI3/auto
        QtQuick.Controls.Windows/auto
        QtQuick.Controls.macOS/auto
        QtQuick.Controls.iOS/auto
    DEFAULT_IMPORTS
        QtQuick.Controls.Basic/auto
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickattachedpropertypropagator.cpp qquickattachedpropertypropagator.h
        qquickstyle.cpp qquickstyle.h qquickstyle_p.h
        qquickstyleplugin.cpp qquickstyleplugin_p.h
        qtquickcontrols2global.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
)

qt_internal_extend_target(qtquickcontrols2plugin
    SOURCES
        qtquickcontrols2plugin.cpp
    LIBRARIES
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickTemplates2Private
)

qt_internal_add_resource(qtquickcontrols2plugin "indirectBasic"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/IndirectBasic"
    FILES
        qmldir
)

if(QT_FEATURE_quick_designer)
    add_subdirectory(designer)
endif()

add_subdirectory(basic)
if(QT_FEATURE_quickcontrols2_fusion)
    add_subdirectory(fusion)
endif()

if(QT_FEATURE_quickcontrols2_imagine)
    add_subdirectory(imagine)
endif()

if(QT_FEATURE_quickcontrols2_material)
    add_subdirectory(material)
endif()

if(QT_FEATURE_quickcontrols2_universal)
    add_subdirectory(universal)
endif()

if(QT_FEATURE_quickcontrols2_fluentwinui3)
    add_subdirectory(fluentwinui3)
endif()

if(QT_FEATURE_quickcontrols2_macos)
    add_subdirectory(macos)
endif()

if(QT_FEATURE_quickcontrols2_ios)
    add_subdirectory(ios)
endif()

if(QT_FEATURE_quickcontrols2_windows)
    add_subdirectory(windows)
endif()

qt_internal_add_docs(QuickControls2
    doc/qtquickcontrols.qdocconf
)

# Make the doc files available to Creator's locator.
file(GLOB doc_files
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/snippets/*.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/src/*.qdoc"
    "${CMAKE_CURRENT_SOURCE_DIR}/doc/src/templates/*.qdoc"
)
target_sources(QuickControls2
    PRIVATE
        ${doc_files}
        doc/qtquickcontrols.qdocconf
)
