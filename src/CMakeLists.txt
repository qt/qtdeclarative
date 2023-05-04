# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from src.pro.
# special case skip regeneration

if (NOT QT_FEATURE_commandlineparser)
    message(FATAL_ERROR "QT_FEATURE_commandlineparser required to compile qmltyperegistrar")
endif()

add_subdirectory(qmltoolingsettings)
add_subdirectory(qmltyperegistrar)
add_subdirectory(qmlintegration)
add_subdirectory(qml)
add_subdirectory(qmlmodels)

add_subdirectory(core)

if(QT_FEATURE_qml_worker_script)
    add_subdirectory(qmlworkerscript)
endif()

if(TARGET Qt::Sql)
   add_subdirectory(qmllocalstorage)
 endif()

if (QT_FEATURE_qml_xmllistmodel)
    add_subdirectory(qmlxmllistmodel)
endif()

if(TARGET Qt::Gui AND TARGET Qt::qsb AND QT_FEATURE_qml_animation)
    add_subdirectory(quick)
    add_subdirectory(quicklayouts)

    find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Test)
    if(QT_FEATURE_testlib AND TARGET Qt::Test)
        add_subdirectory(qmltest)
        if(QT_FEATURE_network)
            add_subdirectory(quicktestutils)
        endif()
    endif()

    if(QT_FEATURE_quick_particles)
        add_subdirectory(particles)
    endif()

    if(QT_FEATURE_quick_shadereffect)
        add_subdirectory(effects)
    endif()

    if(QT_FEATURE_quick_path)
        add_subdirectory(quickshapes)
    endif()

    if(TARGET Qt::Widgets)
        add_subdirectory(quickwidgets)
    endif()

    add_subdirectory(quicktemplates)
    add_subdirectory(quickcontrolsimpl)
    add_subdirectory(quickcontrols)
    add_subdirectory(quickdialogs)
    add_subdirectory(quicknativestyle)

    if(QT_FEATURE_testlib AND QT_FEATURE_network AND TARGET Qt::Test AND TARGET Qt::QuickControls2)
        add_subdirectory(quickcontrolstestutils)
    endif()
else()
    qt_configure_add_summary_section(NAME "Qt Quick")
    qt_configure_add_summary_entry(TYPE "message" ARGS "Qt Quick support" MESSAGE "no")
    qt_configure_add_report_entry(
        TYPE NOTE
        MESSAGE "Qt Quick modules not built due to not finding the qtshadertools 'qsb' tool."
    )
endif()

add_subdirectory(labs)

# special case begin
# These need to be included here since they have dependencies on the modules
# specified above.

add_subdirectory(qmlcompiler)
add_subdirectory(qmldom)

# Build qmlcachegen now, so that we can use it in src/imports.
if(QT_FEATURE_xmlstreamwriter)
    add_subdirectory(../tools/qmlcachegen qmlcachegen)
endif()

if(QT_FEATURE_thread AND TARGET Qt::LanguageServerPrivate AND NOT WASM AND NOT IOS AND NOT ANDROID AND NOT QNX AND NOT INTEGRITY AND NOT WEBOS)
    add_subdirectory(qmlls)
endif()

add_subdirectory(plugins)
add_subdirectory(imports)

if(QT_FEATURE_qml_network)

    if(QT_FEATURE_thread AND QT_FEATURE_localserver AND QT_FEATURE_qml_debug)
        add_subdirectory(qmldebug)
    endif()
endif()
# special case end
