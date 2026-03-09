# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(StyleKitExample VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 6.11 REQUIRED Core Gui Qml Quick QuickControls2)
find_program(QSB_EXECUTABLE qsb HINTS "${QT_HOST_PATH}/bin")
if(NOT QSB_EXECUTABLE OR NOT EXISTS "${QSB_EXECUTABLE}")
    message(FATAL_ERROR "qsb not found. Please install Qt Shader Tools to build this example.")
endif()

qt_standard_project_setup(REQUIRES 6.11)

set(STYLES
    styles/Plain.qml
    styles/Haze.qml
    styles/Experimental.qml
    styles/Vitrum.qml
    styles/CustomDelegates.qml
)

qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}Module
    QML_FILES
        Main.qml
        ${STYLES}
)

qt_add_resources(${PROJECT_NAME} "images"
    PREFIX "/images"
    FILES
        qt.png
)

qt_add_resources(${PROJECT_NAME} "effects"
    PREFIX "/effects"
    FILES
        wave.frag wave.qsb
        noise.frag noise.qsb
)

set(SHADERS
    wave.frag
    noise.frag
)

set(QSB_FLAGS
    --glsl 460,450,320es
    --hlsl 50
    --msl 12
    --msl 20
)

set(GENERATED_SHADERS)
foreach(SHADER ${SHADERS})
    get_filename_component(NAME_WE ${SHADER} NAME_WE)
    set(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${NAME_WE}.qsb)

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${QSB_EXECUTABLE} ${QSB_FLAGS} -o ${OUTPUT} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling shader ${SHADER} → ${NAME_WE}.qsb"
        VERBATIM
    )
    list(APPEND GENERATED_SHADERS ${OUTPUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${GENERATED_SHADERS})
add_dependencies(${PROJECT_NAME} shaders)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
