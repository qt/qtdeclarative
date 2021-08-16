# Generated from views.pro.

cmake_minimum_required(VERSION 3.16)
project(views LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/views")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(views
    main.cpp
)
set_target_properties(views PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(views PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(views_resource_files
    "delegatemodel/dragselection.qml"
    "delegatemodel/slideshow.qml"
    "gridview/gridview-example.qml"
    "gridview/pics/AddressBook_48.png"
    "gridview/pics/AudioPlayer_48.png"
    "gridview/pics/Camera_48.png"
    "gridview/pics/DateBook_48.png"
    "gridview/pics/EMail_48.png"
    "gridview/pics/TodoList_48.png"
    "gridview/pics/VideoPlayer_48.png"
    "listview/content/PetsModel.qml"
    "listview/content/PressAndHoldButton.qml"
    "listview/content/RecipesModel.qml"
    "listview/content/SmallText.qml"
    "listview/content/TextButton.qml"
    "listview/content/ToggleButton.qml"
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
    "pathview/pics/AddressBook_48.png"
    "pathview/pics/AudioPlayer_48.png"
    "pathview/pics/Camera_48.png"
    "pathview/pics/DateBook_48.png"
    "pathview/pics/EMail_48.png"
    "pathview/pics/TodoList_48.png"
    "pathview/pics/VideoPlayer_48.png"
    "views.qml"
)

qt6_add_resources(views "views"
    PREFIX
        "/views"
    FILES
        ${views_resource_files}
)
set(shared_resource_files
    "../shared/Button.qml"
    "../shared/CheckBox.qml"
    "../shared/FlickrRssModel.qml"
    "../shared/Label.qml"
    "../shared/LauncherList.qml"
    "../shared/SimpleLauncherDelegate.qml"
    "../shared/Slider.qml"
    "../shared/TabSet.qml"
    "../shared/TextField.qml"
    "../shared/images/back.png"
    "../shared/images/checkmark.png"
    "../shared/images/next.png"
    "../shared/images/qt-logo.png"
    "../shared/images/slider_handle.png"
    "../shared/images/tab.png"
)

qt6_add_resources(views "shared"
    PREFIX
        "/shared"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

install(TARGETS views
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
