cmake_minimum_required(VERSION 3.16)
project(delegatechooser LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/delegatechooser")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(delegatechooserexample WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(delegatechooserexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

add_dependencies(delegatechooserexample delegatechooser_shared)

qt_add_qml_module(delegatechooserexample
    URI delegatechooser
    VERSION 1.0
    QML_FILES
        "delegatechooser.qml"
)

install(TARGETS delegatechooserexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(delegatechooserexample)
