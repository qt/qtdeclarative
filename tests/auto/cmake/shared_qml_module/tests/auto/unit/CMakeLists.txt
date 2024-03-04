cmake_minimum_required(VERSION 3.5)

project(tst_models LANGUAGES CXX)

enable_testing()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Gui Test Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Test Quick)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tst_models tst_models.cpp)
add_test(NAME tst_models COMMAND tst_models)

target_link_libraries(tst_models
    PRIVATE
        scheduler
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Quick
)

qt_add_qml_module(tst_models
    URI unittest
    DEPENDENCIES
        TARGET scheduler
)

