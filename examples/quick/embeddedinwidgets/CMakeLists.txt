cmake_minimum_required(VERSION 3.16)
project(embeddedinwidgets LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/embeddedinwidgets")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Widgets)

qt_add_executable(embeddedinwidgetsexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(embeddedinwidgetsexample PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Widgets
)

# Resources:
qt_add_qml_module(embeddedinwidgetsexample
    URI embeddedinwidgets
    VERSION 1.0
    QML_FILES
        "main.qml"
    RESOURCES
        "reflect.frag.qsb"
)

install(TARGETS embeddedinwidgetsexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
