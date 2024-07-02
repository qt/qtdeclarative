# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(samegame4 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(samegame4
    main.cpp
)

set_target_properties(samegame4 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

qt_add_qml_module(samegame4
    URI samegame
    QML_FILES
        "BoomBlock.qml"
        "Button.qml"
        "Dialog.qml"
        "samegame.qml"
        "samegame.js"
    RESOURCES
        "highscores/README"
        "highscores/score_data.xml"
        "highscores/score_style.xsl"
        "highscores/scores.php"
        "pics/background.jpg"
        "pics/blueStar.png"
        "pics/blueStone.png"
        "pics/greenStar.png"
        "pics/greenStone.png"
        "pics/redStar.png"
        "pics/redStone.png"
)

target_link_libraries(samegame4 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

install(TARGETS samegame4
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET samegame4
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
