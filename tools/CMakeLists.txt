# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from tools.pro.

add_subdirectory(qmldom)
if(QT_FEATURE_commandlineparser)
    add_subdirectory(qmllint)
    add_subdirectory(qmltc)
    add_subdirectory(qmltyperegistrar)
endif()
add_subdirectory(qmlimportscanner)
add_subdirectory(qmlformat)
if (TARGET Qt::LanguageServerPrivate AND QT_FEATURE_commandlineparser
        AND NOT WASM AND NOT IOS AND NOT ANDROID AND NOT QNX AND NOT INTEGRITY AND NOT WEBOS)
    add_subdirectory(qmlls)
endif()
if(QT_FEATURE_xmlstreamwriter)
    # special case begin
    # Do not build qmlcachegen here but build it at src/
    # time, so that we can use it for our own .qml files in src/imports.
    # add_subdirectory(qmlcachegen)
    # special case end
endif()
if(QT_FEATURE_thread AND NOT ANDROID AND NOT WASM AND NOT IOS AND NOT rtems)
    add_subdirectory(qml)
endif()
if(QT_FEATURE_qml_profiler
        AND QT_FEATURE_thread
        AND NOT ANDROID
        AND NOT IOS
        AND NOT WASM
        AND NOT rtems)
    add_subdirectory(qmlprofiler)
endif()
if(QT_FEATURE_qml_preview AND QT_FEATURE_thread AND NOT ANDROID AND NOT WASM AND NOT IOS AND NOT rtems)
    add_subdirectory(qmlpreview)
endif()
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_thread AND TARGET Qt::Quick AND NOT ANDROID AND NOT WASM AND NOT IOS AND NOT rtems)
    add_subdirectory(qmlscene)
    add_subdirectory(qmltime)
endif()
if(QT_BUILD_SHARED_LIBS
        AND QT_FEATURE_process
        AND QT_FEATURE_regularexpression
        AND QT_FEATURE_thread
        AND TARGET Qt::Quick
        AND NOT ANDROID
        AND NOT IOS
        AND NOT WASM
        AND NOT rtems)
    add_subdirectory(qmlplugindump)
endif()
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
        AND QT_FEATURE_textedit
        AND NOT ANDROID
        AND NOT IOS
        AND NOT WASM
        AND NOT rtems)
    add_subdirectory(qmleasing)
endif()
if(QT_FEATURE_thread AND TARGET Qt::QuickTest AND NOT ANDROID AND NOT WASM AND NOT rtems)
    add_subdirectory(qmltestrunner)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_thread AND NOT ANDROID AND NOT WASM AND NOT IOS AND NOT rtems)
    add_subdirectory(qmljs)
endif()
if (QT_FEATURE_private_tests AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qmljsrootgen)
endif()
