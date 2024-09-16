# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(text LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../shared" "shared")

qt_add_executable(textexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(textexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(textexample text_shared)

qt_add_qml_module(textexample
    URI text
    QML_FILES
        "fonts/availableFonts.qml"
        "fonts/banner.qml"
        "fonts/fonts.qml"
        "fonts/hello.qml"
        "imgtag/TextWithImage.qml"
        "imgtag/imgtag.qml"
        "styledtext-layout.qml"
        "text.qml"
    RESOURCES
        "fonts/content/fonts/tarzeau_ocr_a.ttf"
        "imgtag/images/face-sad.png"
        "imgtag/images/face-smile-big.png"
        "imgtag/images/face-smile.png"
        "imgtag/images/heart200.png"
        "imgtag/images/qtlogo.png"
        "imgtag/images/starfish_2.png"
)

install(TARGETS textexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET textexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
