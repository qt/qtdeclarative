cmake_minimum_required(VERSION 3.16)

project(tst_scheduler_app VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Test)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(tst_scheduler_app
    tst_scheduler_app.cpp
)

qt_add_qml_module(tst_scheduler_app
    URI p1.p2.p3.SchedulerApp
    DEPENDENCIES
        TARGET scheduler
    IMPORTS
        TARGET nested_module
    VERSION 1.0
    QML_FILES
        Main.qml
)

set_target_properties(tst_scheduler_app PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(tst_scheduler_app
    PRIVATE
        Qt::Quick
        Qt::Test
)
