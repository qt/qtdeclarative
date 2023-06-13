# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(text LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/text")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(textexample)
