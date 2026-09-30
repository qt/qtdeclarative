# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_urlinterception LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generate a module of composite types that refer to each other by name, like a large application
# does. Each type name resolved in a document runs the URL interceptors (QTBUG-147851). Types only
# refer to types with higher numbers so that there are no cyclic dependencies.
set(type_count 100)
set(reference_count 10)

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(qmldir_content "module App\n")
set(main_content "import QtQml\nimport App\n\nQtObject {\n")
set(generated_files "${generated_dir}/App/qmldir" "${generated_dir}/Main.qml")

math(EXPR last_type "${type_count} - 1")
foreach(type RANGE ${last_type})
    set(type_content "import QtQml\n\nQtObject {\n")
    foreach(reference RANGE 1 ${reference_count})
        math(EXPR referenced_type "${type} + ${reference}")
        if(referenced_type LESS type_count)
            string(APPEND type_content
                "    property Type${referenced_type} reference${reference}: null\n")
        endif()
    endforeach()
    string(APPEND type_content "}\n")

    file(CONFIGURE OUTPUT "${generated_dir}/App/Type${type}.qml" CONTENT "${type_content}" @ONLY)
    list(APPEND generated_files "${generated_dir}/App/Type${type}.qml")
    string(APPEND qmldir_content "Type${type} 1.0 Type${type}.qml\n")
    string(APPEND main_content "    property QtObject object${type}: Type${type} {}\n")
endforeach()

string(APPEND main_content "}\n")
file(CONFIGURE OUTPUT "${generated_dir}/App/qmldir" CONTENT "${qmldir_content}" @ONLY)
file(CONFIGURE OUTPUT "${generated_dir}/Main.qml" CONTENT "${main_content}" @ONLY)

# An unrelated resource root, registered many times at run time to model an application that links
# many libraries with resources of their own. Each qrc lookup has to go through all of them.
file(CONFIGURE OUTPUT "${generated_dir}/other/qmldir" CONTENT "module Other\n" @ONLY)
file(CONFIGURE OUTPUT "${generated_dir}/other/other.qrc" CONTENT
    "<RCC><qresource prefix=\"/qt/qml/Other\"><file>qmldir</file></qresource></RCC>\n" @ONLY)

qt_internal_add_benchmark(tst_urlinterception
    SOURCES
        tst_urlinterception.cpp
    DEFINES
        GENERATED_DIR="${generated_dir}"
        OTHER_RCC="${CMAKE_CURRENT_BINARY_DIR}/other.rcc"
        TYPE_COUNT=${type_count}
    LIBRARIES
        Qt::Qml
        Qt::Test
)

qt_internal_add_resource(tst_urlinterception "generated"
    PREFIX
        "/qt/qml"
    BASE
        "${generated_dir}"
    FILES
        ${generated_files}
)

qt_add_binary_resources(tst_urlinterception_other "${generated_dir}/other/other.qrc"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/other.rcc"
    OPTIONS -binary
)
add_dependencies(tst_urlinterception tst_urlinterception_other)
