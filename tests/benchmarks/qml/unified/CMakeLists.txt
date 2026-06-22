# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Unified QML benchmark suite.
##
## Gathers QML-related benchmarks from across the code base and qmlbench and runs
## them in several compile/load modes (compile from source, load run-time
## .qmlc/.jsc/.mjsc, load AOT units baked into the binary) crossed with
## object-creation and actual-benchmark phases.
##
## Per logical bundle of documents (a subdirectory of groups/) there are two
## qt_add_qml_module targets, created by unified_add_bundle():
##   * b<N>_s - NO_CACHEGEN: documents packed into the qrc as source
##                   only, for compile-from-source and run-time-.qmlc modes.
##                   URI Bundle.src.<group>.
##   * b<N>_a - cachegen on: documents compiled ahead of time.
##                   URI Bundle.aot.<group>.
## Documents follow the standard user-module path
## :/qt/qml/Bundle/{src,aot}/<group>/<Name>.qml and are loaded with
## QQmlComponent::loadFromModule() where it makes sense (warm create/run).
##
## QML_DISK_CACHE is read once per process, so each mode is a separate executable
## that sets the environment at startup (see modes/). Adding a mode means adding
## an executable; adding documents means dropping a directory in groups/ and
## listing it via add_subdirectory below.
#####################################################################

qt_standard_project_setup(REQUIRES 6.12)

# Create a static QML module whose backing library and plugin run moc. qt_add_qml_module's own
# library would look for moc at its final location in the build, which doesn't exist yet when
# configuring a fresh Qt build. Set up the targets like other in-tree tests do, so that they depend
# on the moc target instead.
function(unified_add_qml_module target)
    qt_add_library(${target} STATIC)
    qt_autogen_tools_initial_setup(${target})
    qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS uic)
    qt_add_qml_module(${target} ${ARGN})
    qt_autogen_tools_initial_setup(${target}plugin)
    qt_autogen_tools(${target}plugin ENABLE_AUTOGEN_TOOLS uic)
    qt_autogen_tools(${target}plugin_init ENABLE_AUTOGEN_TOOLS uic)
    qt_autogen_tools(${target}plugin_init ENABLE_AUTOGEN_TOOLS moc)
endfunction()


add_subdirectory(qmlbench)
add_subdirectory(testtypes)

set_property(GLOBAL PROPERTY UNIFIED_SRC_BUNDLES "")
set_property(GLOBAL PROPERTY UNIFIED_AOT_BUNDLES "")
set_property(GLOBAL PROPERTY UNIFIED_BUNDLE_COUNT 0)

# Create the two qml-module targets for one logical bundle. The targets are numbered rather than
# named after the group: the generated file names repeat the target name, and with the group in it
# they exceed the Windows path length limit. Call from the group's own CMakeLists so QML_FILES are
# local (no nested directory in the resource).
function(unified_add_bundle group)
    get_property(count GLOBAL PROPERTY UNIFIED_BUNDLE_COUNT)
    math(EXPR count "${count} + 1")
    set_property(GLOBAL PROPERTY UNIFIED_BUNDLE_COUNT ${count})
    set(id b${count})
    # .js/.mjs are picked up too so documents that pull in a companion script
    # (e.g. import "deltablue.js") carry it inside the same module.
    file(GLOB qml_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.qml" "*.js" "*.mjs")

    # Plugin-backed STATIC modules. The backing lib is static, so its generated
    # plugin is a static lib too. Linking the *plugin* target propagates the
    # plugin's init object library, whose constructor carries the QT_KEEP_RESOURCE
    # reference to the qmlcache resource — that is what stops the linker from
    # discarding the qmlcache loader (and thus the ahead-of-time-compiled units)
    # of a directly-linked static module. So the executables must link the plugin
    # targets, not just the backing libraries.
    unified_add_qml_module(${id}_s
        URI "Bundle.src.${group}"
        CLASS_NAME ${id}_s
        VERSION 1.0
        NO_CACHEGEN
        NO_LINT
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Bundle/src/${group}"
        QML_FILES ${qml_files}
        DEPENDENCIES QmlBench TestTypes
    )
    target_link_libraries(${id}_s PUBLIC Qt::Quick BenchmarkQmlBench BenchmarkTestTypes)

    unified_add_qml_module(${id}_a
        URI "Bundle.aot.${group}"
        CLASS_NAME ${id}_a
        VERSION 1.0
        NO_LINT
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Bundle/aot/${group}"
        QML_FILES ${qml_files}
        DEPENDENCIES QmlBench TestTypes
    )
    target_link_libraries(${id}_a PUBLIC Qt::Quick BenchmarkQmlBench BenchmarkTestTypes)

    set_property(GLOBAL APPEND PROPERTY UNIFIED_SRC_BUNDLES
        ${id}_s ${id}_splugin)
    set_property(GLOBAL APPEND PROPERTY UNIFIED_AOT_BUNDLES
        ${id}_a ${id}_aplugin)
endfunction()

# --- Logical bundles (one directory under groups/ each). ---
add_subdirectory(groups/creation_basictypes)
add_subdirectory(groups/creation_quick_basictypes)
add_subdirectory(groups/creation_quick)
add_subdirectory(groups/creation_quick_item)
add_subdirectory(groups/creation_quick_rectangle)
add_subdirectory(groups/creation_quick_transforms)
add_subdirectory(groups/creation_quick_text)
add_subdirectory(groups/creation_quick_validators)
add_subdirectory(groups/creation_modelaccess)
add_subdirectory(groups/creation_controls)
add_subdirectory(groups/creation_layouts)
add_subdirectory(groups/changes)
add_subdirectory(groups/cppcodegen)
add_subdirectory(groups/deltablue)
add_subdirectory(groups/eval)
add_subdirectory(groups/examples)
add_subdirectory(groups/js)
add_subdirectory(groups/js_v8bench)
add_subdirectory(groups/bindings)

get_property(src_bundles GLOBAL PROPERTY UNIFIED_SRC_BUNDLES)
get_property(aot_bundles GLOBAL PROPERTY UNIFIED_AOT_BUNDLES)

# --- Mode executables. Each links the matching bundles and the QmlBench module,
#     sets its disk-cache environment in main(), and runs the shared harness.
#     Adding a future mode is just another call here. ---
function(unified_add_mode name mainfile bundles)
    qt_internal_add_benchmark(${name}
        SOURCES
            ${mainfile}
            benchmarkharness.cpp benchmarkharness.h
            coldbenchmark.h
        LIBRARIES
            Qt::CorePrivate
            Qt::Gui
            Qt::Qml
            Qt::QmlPrivate
            Qt::Quick
            Qt::Test
            ${bundles}
            BenchmarkQmlBench
            BenchmarkQmlBenchplugin
            BenchmarkTestTypes
            BenchmarkTestTypesplugin
    )
    qt_autogen_tools(${name} ENABLE_AUTOGEN_TOOLS uic)
    qt_autogen_tools(${name} ENABLE_AUTOGEN_TOOLS moc)
endfunction()

unified_add_mode(tst_unified_source modes/main_source.cpp "${src_bundles}")
unified_add_mode(tst_unified_qmlc   modes/main_qmlc.cpp   "${src_bundles}")
unified_add_mode(tst_unified_aot    modes/main_aot.cpp    "${aot_bundles}")
