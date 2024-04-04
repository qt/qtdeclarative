# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(metalunderqml LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(metalunderqml
    main.cpp
    metalsquircle.h metalsquircle.mm
)

set_target_properties(metalunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(metalunderqml PUBLIC
    "-framework Metal"
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

if(MACOS)
    target_link_libraries(metalunderqml PUBLIC
        "-framework AppKit"
    )
endif()

qt_add_qml_module(metalunderqml
    URI MetalUnderQML
    QML_FILES
        main.qml
    RESOURCES
        squircle.frag
        squircle.vert
    RESOURCE_PREFIX /scenegraph/metalunderqml
    NO_RESOURCE_TARGET_PATH
    )

install(TARGETS metalunderqml
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET metalunderqml
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
