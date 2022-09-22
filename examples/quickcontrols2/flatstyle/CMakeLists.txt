cmake_minimum_required(VERSION 3.16)
project(flatstyle LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/flatstyle")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Widgets)

qt_add_executable(flatstyleexample WIN32 MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(flatstyleexample
    URI flatstyle
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "MainForm.ui.qml"
        "flatstyle.qml"
        "imports/Flat/Button.qml"
        "imports/Flat/CheckBox.qml"
        "imports/Flat/Switch.qml"
        "imports/Theme/Theme.qml"
    RESOURCES
        "imports/Flat/qmldir"
        "imports/Theme/qmldir"
        "qtquickcontrols2.conf"
)

target_link_libraries(flatstyleexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
    Qt::Widgets
)

install(TARGETS flatstyleexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
