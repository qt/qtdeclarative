# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(metaltextureimport LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/metaltextureimport")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(metaltextureimport
    main.cpp
    metaltextureimport.h metaltextureimport.mm
)

set_target_properties(metaltextureimport PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE

    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME metaltextureimportapp
)

target_link_libraries(metaltextureimport PUBLIC
    "-framework Metal"
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

if(MACOS)
    target_link_libraries(metaltextureimport PUBLIC
        "-framework AppKit"
    )
endif()

qt_add_qml_module(metaltextureimport
    URI MetalTextureImport
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag
        squircle.vert
    RESOURCE_PREFIX /scenegraph/metaltextureimport
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS metaltextureimport
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
