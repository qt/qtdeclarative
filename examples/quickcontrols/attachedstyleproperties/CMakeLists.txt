cmake_minimum_required(VERSION 3.16)
project(attachedstyleproperties LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/attachedstyleproperties")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(MyStyle)

qt_add_executable(attachedstylepropertiesexample
    WIN32
    MACOSX_BUNDLE
    attachedstyleproperties.cpp
)

qt_add_qml_module(attachedstylepropertiesexample
    URI App
    QML_FILES
        attachedstyleproperties.qml
    NO_RESOURCE_TARGET_PATH
)

target_link_libraries(attachedstylepropertiesexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::QuickControls2
)

add_dependencies(attachedstylepropertiesexample MyStyle)

install(TARGETS attachedstylepropertiesexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

include(../../quick/shared/QtBundleQmlModuleForMacOS.cmake)

set(app_target "attachedstylepropertiesexample")
set(qml_plugin_target "MyStyleplugin")
set(qml_module_uri "MyStyle")
add_qml_module_to_macos_app_bundle(
    "${app_target}" "${qml_plugin_target}" "${qml_module_uri}")
