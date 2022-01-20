cmake_minimum_required(VERSION 3.16)
project(qml_pixelator LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/tableview/pixelator")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(qml_pixelator
    imagemodel.cpp imagemodel.h
    main.cpp
)

set_target_properties(qml_pixelator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qml_pixelator PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(qml_pixelator
    URI ImageModel
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES qt.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS qml_pixelator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
