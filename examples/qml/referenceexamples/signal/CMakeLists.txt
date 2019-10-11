# Generated from signal.pro.

cmake_minimum_required(VERSION 3.14)
project(signal LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qml/referenceexamples/signal")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(signal
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
target_link_libraries(signal PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)


# Resources:
set(signal_resource_files
    "example.qml"
)

qt6_add_resources(signal "signal"
    PREFIX
        "/"
    FILES
        ${signal_resource_files}
)

install(TARGETS signal
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
