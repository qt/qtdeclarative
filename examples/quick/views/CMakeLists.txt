# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(views LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../shared" "shared")

qt_add_executable(viewsexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(viewsexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(viewsexample views_shared)

qt_add_qml_module(viewsexample
    URI views
    QML_FILES
        "delegatemodel/dragselection.qml"
        "gridview/gridview-example.qml"
        "listview/content/PetsModel.qml"
        "listview/content/PressAndHoldButton.qml"
        "listview/content/RecipesModel.qml"
        "listview/content/SmallText.qml"
        "listview/content/TextButton.qml"
        "listview/content/ToggleButton.qml"
        "listview/displaymargin.qml"
        "listview/dynamiclist.qml"
        "listview/expandingdelegates.qml"
        "listview/highlight.qml"
        "listview/highlightranges.qml"
        "listview/sections.qml"
        "objectmodel/objectmodel.qml"
        "package/Delegate.qml"
        "package/view.qml"
        "pathview/pathview-example.qml"
        "views.qml"
    RESOURCES
        "gridview/pics/AddressBook_48.png"
        "gridview/pics/AudioPlayer_48.png"
        "gridview/pics/Camera_48.png"
        "gridview/pics/DateBook_48.png"
        "gridview/pics/EMail_48.png"
        "gridview/pics/TodoList_48.png"
        "gridview/pics/VideoPlayer_48.png"
        "listview/content/pics/arrow-down.png"
        "listview/content/pics/arrow-up.png"
        "listview/content/pics/fruit-salad.jpg"
        "listview/content/pics/hamburger.jpg"
        "listview/content/pics/lemonade.jpg"
        "listview/content/pics/list-delete.png"
        "listview/content/pics/minus-sign.png"
        "listview/content/pics/moreDown.png"
        "listview/content/pics/moreUp.png"
        "listview/content/pics/pancakes.jpg"
        "listview/content/pics/plus-sign.png"
        "listview/content/pics/vegetable-soup.jpg"
        "pathview/pics/AddressBook_48.png"
        "pathview/pics/AudioPlayer_48.png"
        "pathview/pics/Camera_48.png"
        "pathview/pics/DateBook_48.png"
        "pathview/pics/EMail_48.png"
        "pathview/pics/TodoList_48.png"
        "pathview/pics/VideoPlayer_48.png"
)

install(TARGETS viewsexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET viewsexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
