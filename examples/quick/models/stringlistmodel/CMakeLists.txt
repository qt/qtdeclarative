cmake_minimum_required(VERSION 3.16)
project(stringlistmodel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/models/stringlistmodel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(stringlistmodelexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(stringlistmodelexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
qt_add_qml_module(stringlistmodelexample
    URI stringlistmodel
    VERSION 1.0
    QML_FILES
        "view.qml"
)

install(TARGETS stringlistmodelexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
