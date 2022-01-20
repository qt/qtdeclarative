cmake_minimum_required(VERSION 3.16)
project(abstractitemmodel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/models/abstractitemmodel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(abstractitemmodelexample WIN32 MACOSX_BUNDLE
    main.cpp
    model.cpp
)

target_link_libraries(abstractitemmodelexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
qt_add_qml_module(abstractitemmodelexample
    URI abstractitemmodel
    VERSION 1.0
    QML_FILES
        "view.qml"
)

install(TARGETS abstractitemmodelexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
