cmake_minimum_required(VERSION 3.16)
project(positioners LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/positioners")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(positionersexample
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(positionersexample
    URI positioners
    VERSION 1.0
    QML_FILES
        "positioners-attachedproperties.qml"
        "positioners-transitions.qml"
        "positioners.qml"
)

target_link_libraries(positionersexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS positionersexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(positionersexample)
