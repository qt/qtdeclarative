# Generated from xmlhttprequest.pro.

cmake_minimum_required(VERSION 3.16)
project(xmlhttprequest LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qml/xmlhttprequest")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(xmlhttprequest
    main.cpp
)
set_target_properties(xmlhttprequest PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(xmlhttprequest PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml)
qt_add_qml_module(xmlhttprequest
    URI xmlhttprequest
    VERSION 1.0
    QML_FILES
        Get.qml
        GetForm.ui.qml
        methods.js
        xmlhttprequest.qml
    RESOURCES
        data.xml
    RESOURCE_PREFIX /qml
)

# These files live outside of the QML module, so add them separately
set(quick_shared_resource_files
    "../../quick/shared/Button.qml"
    "../../quick/shared/CheckBox.qml"
    "../../quick/shared/Label.qml"
    "../../quick/shared/LauncherList.qml"
    "../../quick/shared/SimpleLauncherDelegate.qml"
    "../../quick/shared/Slider.qml"
    "../../quick/shared/TextField.qml"
    "../../quick/shared/images/back.png"
    "../../quick/shared/images/checkmark.png"
    "../../quick/shared/images/next.png"
    "../../quick/shared/images/slider_handle.png"
)

qt6_add_resources(xmlhttprequest "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../quick/shared"
    FILES
        ${quick_shared_resource_files}
)

install(TARGETS xmlhttprequest
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
