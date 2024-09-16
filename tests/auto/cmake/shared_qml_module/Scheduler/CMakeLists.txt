cmake_minimum_required(VERSION 3.16)

project(scheduler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_qml_module(scheduler
    URI Scheduler
    VERSION 1.0
    SOURCES
        task.h
        task.cpp
        schedulerglobal.h
    QML_FILES
        MainScreen.qml
)

target_compile_definitions(scheduler
    PRIVATE
        SCHEDULER_LIBRARY
)

target_include_directories(scheduler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(scheduler
    PRIVATE
        Qt6::Quick
)
