# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from tools.pro.

add_subdirectory(qmldom)
if(QT_FEATURE_commandlineparser)
    add_subdirectory(qmllint)
    add_subdirectory(qmltc)
    add_subdirectory(qmltyperegistrar)
    add_subdirectory(qmljsrootgen)
endif()
add_subdirectory(qmlimportscanner)
add_subdirectory(qmlformat)
if(TARGET Qt::LanguageServerPrivate AND QT_FEATURE_commandlineparser AND QT_FEATURE_filesystemwatcher)
    if (NOT CMAKE_CROSSCOMPILING OR QT_FORCE_BUILD_TOOLS)
        add_subdirectory(qmlls)
    endif()
endif()
if(QT_FEATURE_xmlstreamwriter)
    # special case begin
    # Do not build qmlcachegen here but build it at src/
    # time, so that we can use it for our own .qml files in src/imports.
    # add_subdirectory(qmlcachegen)
    # special case end
endif()

if(NOT (ANDROID OR WASM OR IOS OR VISIONOS OR rtems))
    if(QT_FEATURE_thread)
        add_subdirectory(qml)
        if(QT_FEATURE_qml_profiler)
            add_subdirectory(qmlprofiler)
        endif()
        if(QT_FEATURE_qml_preview)
            add_subdirectory(qmlpreview)
        endif()
        if(QT_BUILD_SHARED_LIBS AND TARGET Qt::Quick)
            add_subdirectory(qmlscene)
            add_subdirectory(qmltime)
        endif()
        if(QT_BUILD_SHARED_LIBS
            AND QT_FEATURE_process
            AND QT_FEATURE_regularexpression
            AND TARGET Qt::Quick)
            add_subdirectory(qmlplugindump)
        endif()
        if(TARGET Qt::QuickTest)
            add_subdirectory(qmltestrunner)
        endif()
        if(QT_FEATURE_private_tests)
            add_subdirectory(qmljs)
        endif()
    endif() # QT_FEATURE_thread

    if(TARGET Qt::Quick
            AND TARGET Qt::Widgets
            AND QT_FEATURE_checkbox
            AND QT_FEATURE_combobox
            AND QT_FEATURE_dialogbuttonbox
            AND QT_FEATURE_formlayout
            AND QT_FEATURE_groupbox
            AND QT_FEATURE_lineedit
            AND QT_FEATURE_mainwindow
            AND QT_FEATURE_spinbox
            AND QT_FEATURE_textedit)
        add_subdirectory(qmleasing)
    endif()

    if(TARGET Qt::Quick AND TARGET Qt::Svg)
        add_subdirectory(svgtoqml)
    endif()
endif() # NOT (ANDROID OR WASM OR IOS OR rtems)
