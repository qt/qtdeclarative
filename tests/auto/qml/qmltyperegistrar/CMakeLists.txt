# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltyperegistrar.pro.

#####################################################################
## tst_qmltyperegistrar Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmltyperegistrar LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

add_subdirectory(VersionZero)
add_subdirectory(UnregisteredTypes)
add_subdirectory(foreign)
add_subdirectory(Namespaced)

qt_manual_moc(moc_files OUTPUT_MOC_JSON_FILES json_list noextheader
              TARGETS Qt6::Qml)

# Dummy target to pass --private-includes to qmltyperegistrar for tst_qmltyperegistrar.
# We want to test that it expects files named foo_p.h appearing in foreign metatypes
# to be in a private/ subdirectory. In particular, we have a foreign_p.h to exercise this.
# TODO: There should be more fine grained control over this.
qt_add_library(tst_qmltyperegistrarPrivate STATIC
    dummy.cpp dummy_p.h
)

target_link_libraries(tst_qmltyperegistrarPrivate PRIVATE Qt::Core)

qt_internal_add_test(tst_qmltyperegistrar
    SOURCES
        hppheader.hpp
        typereferencinganother.h
        # noextheader special case
        tst_qmltyperegistrar.cpp tst_qmltyperegistrar.h
        foo.cpp foo.h duplicatedExports.h
        inaccessible/base.h
        inaccessible/property.h
        ${moc_files}
    INCLUDE_DIRECTORIES
        foreign
        Namespaced                               # for namespaced.h
        "${CMAKE_CURRENT_BINARY_DIR}/Namespaced" # for the export header
    LIBRARIES
        # Remove: Lforeign
        Qt::Qml
        Qt::QmlPrivate
        foreign
        tst_qmltyperegistrarPrivate
        Qt::QmlTypeRegistrarPrivate
        tst_qmltyperegistrar_major_version_zero
        tst_qmltyperegistrar_namespaced_module
        UnregisteredTypes
)

if (TARGET qmljsrootgen)
    qt_internal_extend_target(tst_qmltyperegistrar
        DEFINES
            QT_QMLJSROOTGEN_PRESENT
    )

    add_dependencies(tst_qmltyperegistrar Qt::qmljsrootgen)
endif()

qt_internal_extend_target(tst_qmltyperegistrar CONDITION TARGET Qt::Quick
    LIBRARIES
        Qt::Quick
    )

#### Keys ignored in scope 2:.:.:tst_qmltyperegistrar.pro:<TRUE>:
# QMLTYPES_FILENAME = "tst_qmltyperegistrar.qmltypes"
# QML_FOREIGN_METATYPES = "foreign/foreign_metatypes.json"
# QML_IMPORT_NAME = "QmlTypeRegistrarTest"
# QML_IMPORT_VERSION = "1.0"
# QML_PAST_MAJOR_VERSIONS = "0"
# TEMPLATE = "app"

## Scopes:
#####################################################################

# Simulate conditions that qt6_add_qml_module() would normally set up for us
set_target_properties(tst_qmltyperegistrar PROPERTIES
    QT_QML_MODULE_VERSION 1.1
    QT_QML_MODULE_PAST_MAJOR_VERSIONS 0
    QT_QML_MODULE_URI QmlTypeRegistrarTest
    QT_QML_MODULE_TYPEINFO tst_qmltyperegistrar.qmltypes
    QT_QML_MODULE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# qt6_add_qml_module() doesn't allow us to pass through MANUAL_MOC_JSON_FILES
# yet, so we have to call it directly to test that code path for now.
_qt_internal_qml_type_registration(tst_qmltyperegistrar MANUAL_MOC_JSON_FILES ${json_list})

qt_add_library(tst-qmltyperegistrar-with-dashes)
qt_autogen_tools_initial_setup(tst-qmltyperegistrar-with-dashes)
target_link_libraries(tst-qmltyperegistrar-with-dashes PRIVATE Qt::Core Qt::Qml)
qt_enable_autogen_tool(tst-qmltyperegistrar-with-dashes "moc" ON)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst-qmltyperegistrar-with-dashes
    URI Module-With-Dashes
    SOURCES
        foo.cpp foo.h
    OUTPUT_DIRECTORY Module-With-Dashes
)
qt_autogen_tools_initial_setup(tst-qmltyperegistrar-with-dashesplugin)

qt_internal_add_resource(tst_qmltyperegistrar "resources"
    PREFIX
        "/"
    FILES
        brokenEnums.json
        duplicatedExports.json
        missingTypes.json
        processTwice.json
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
qt_add_library(tst-qmltyperegistrar-enum-foreign STATIC enum.cpp)
qt_autogen_tools_initial_setup(tst-qmltyperegistrar-enum-foreign)
qt_enable_autogen_tool(tst-qmltyperegistrar-enum-foreign "moc" ON)
target_link_libraries(tst-qmltyperegistrar-enum-foreign PRIVATE Qt::QmlIntegration)

qt_add_library(tst-qmltyperegistrar-enum STATIC)
qt_autogen_tools_initial_setup(tst-qmltyperegistrar-enum)
qt_enable_autogen_tool(tst-qmltyperegistrar-enum "moc" ON)
target_link_libraries(tst-qmltyperegistrar-enum PRIVATE Qt::Qml tst-qmltyperegistrar-enum-foreign)

qt_add_qml_module(tst-qmltyperegistrar-enum URI TstEnum OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TstEnum)
qt_autogen_tools_initial_setup(tst-qmltyperegistrar-enumplugin)
qt_generate_foreign_qml_types(tst-qmltyperegistrar-enum-foreign tst-qmltyperegistrar-enum)
