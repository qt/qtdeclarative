# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.26)
project(helloswift LANGUAGES CXX Swift)

# --------------- Qt ----------------

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup(REQUIRES 6.5)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/helloswift")

qt_add_executable(helloswift WIN32 MACOSX_BUNDLE
    main.cpp
)

target_include_directories(helloswift PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(helloswift PRIVATE
    Qt6::Quick
)

qt_add_qml_module(helloswift
    URI helloswift
    QML_FILES
        "main.qml"
)

install(TARGETS helloswift
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

# ------------- Swift ---------------

# Verify that we have a new enough compiler
if("${CMAKE_Swift_COMPILER_VERSION}" VERSION_LESS 5.9)
  message(FATAL_ERROR "Swift 5.9 required for C++ interoperability")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
set(CMAKE_CXX_STANDARD 17)

get_target_property(QT_CORE_INCLUDES Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)

# Swift library
set(SWIFT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/hello.swift")
add_library(hello-swift STATIC ${SWIFT_SOURCES})
set_target_properties(hello-swift PROPERTIES Swift_MODULE_NAME "HelloSwift")
target_include_directories(hello-swift PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${QT_CORE_INCLUDES}"
)
target_compile_options(hello-swift PUBLIC
  $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
  $<$<COMPILE_LANGUAGE:Swift>:-Xcc -std=c++17>)

# Swift to C++ bridging header
set(SWIFT_BRIDGING_HEADER "${CMAKE_CURRENT_BINARY_DIR}/hello-swift.h")
list(TRANSFORM QT_CORE_INCLUDES PREPEND "-I")
add_custom_command(
    COMMAND
      ${CMAKE_Swift_COMPILER} -frontend -typecheck
      ${SWIFT_SOURCES}
      -I ${CMAKE_CURRENT_SOURCE_DIR}
      ${QT_CORE_INCLUDES}
      -sdk ${CMAKE_OSX_SYSROOT}
      -module-name HelloSwift
      -cxx-interoperability-mode=default
      -Xcc -std=c++17
      -emit-clang-header-path "${SWIFT_BRIDGING_HEADER}"
    OUTPUT
       "${SWIFT_BRIDGING_HEADER}"
    DEPENDS
        ${SWIFT_SOURCES}
  )

add_custom_target(swift-bridging-header
    DEPENDS
        "${SWIFT_BRIDGING_HEADER}"
)

add_dependencies(hello-swift swift-bridging-header)

# Tie everything together

target_link_libraries(helloswift PRIVATE
    hello-swift
)
