# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from src.pro.
# special case skip regeneration

if(NOT QT_FEATURE_commandlineparser OR NOT QT_FEATURE_regularexpression)
    message(FATAL_ERROR "QT_FEATURE_commandlineparser and QT_FEATURE_regularexpression required to compile qmltyperegistrar")
endif()

add_subdirectory(qmltoolingsettings)
add_subdirectory(qmltyperegistrar)
add_subdirectory(qmlintegration)
add_subdirectory(qml)
add_subdirectory(qmlmodels)

add_subdirectory(core)

if(QT_FEATURE_qml_network)
    add_subdirectory(qmlnetwork)
endif()

if(QT_FEATURE_qml_worker_script)
    add_subdirectory(qmlworkerscript)
endif()

add_subdirectory(qmlmeta)

if(TARGET Qt::Sql)
   add_subdirectory(qmllocalstorage)
 endif()

if(QT_FEATURE_qml_xmllistmodel)
    add_subdirectory(qmlxmllistmodel)
endif()

if(TARGET Qt::Gui AND TARGET Qt::qsb AND QT_FEATURE_qml_animation)
    add_subdirectory(quick)
    add_subdirectory(quicklayouts)

    find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Test)
    if(QT_FEATURE_testlib AND TARGET Qt::Test)
        add_subdirectory(qmltest)
        if(QT_FEATURE_network AND QT_FEATURE_quick_itemview)
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

    if(TARGET Qt::Svg)
        add_subdirectory(quickvectorimage)
    endif()

    if(TARGET Qt::Widgets)
        add_subdirectory(quickwidgets)
    endif()

    add_subdirectory(quicktemplates)
    add_subdirectory(quickcontrolsimpl)
    add_subdirectory(quickcontrols)
    if(QT_FEATURE_quicktemplates2_container)
        add_subdirectory(quickdialogs)
    endif()
    if (QT_FEATURE_quickcontrols2_macos OR QT_FEATURE_quickcontrols2_windows)
        add_subdirectory(quicknativestyle)
    endif()

    if(QT_FEATURE_testlib AND QT_FEATURE_network AND QT_FEATURE_quick_itemview AND TARGET Qt::Test AND TARGET Qt::QuickControls2 AND NOT (WASM AND BUILD_SHARED_LIBS))
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

if (QT_FEATURE_commandlineparser)
    add_subdirectory(qmlformat)
endif()

# Build qmlcachegen now, so that we can use it in src/imports.
if(QT_FEATURE_xmlstreamwriter)
    add_subdirectory(../tools/qmlaotstats qmlaotstats)
    add_subdirectory(../tools/qmlcachegen qmlcachegen)
endif()

if(QT_FEATURE_thread AND QT_FEATURE_filesystemwatcher AND QT_FEATURE_processenvironment
        AND TARGET Qt::LanguageServerPrivate)
    if (NOT CMAKE_CROSSCOMPILING OR QT_FORCE_BUILD_TOOLS)
        add_subdirectory(qmlls)
    endif()
endif()

add_subdirectory(plugins)
add_subdirectory(imports)

if(QT_FEATURE_qml_network)

    if(QT_FEATURE_thread AND QT_FEATURE_localserver AND QT_FEATURE_qml_debug)
        add_subdirectory(qmldebug)
    endif()
endif()

# special case end
