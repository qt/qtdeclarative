cmake_minimum_required(VERSION 3.16)
project(attachedstyleproperties LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET attachedstylepropertiesexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
