cmake_minimum_required(VERSION 3.21)

project(hello_world_plugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS QmlCompiler)

qt_standard_project_setup(
    REQUIRES 6.8
)

qt_add_plugin(HelloWorldPlugin)

target_sources(HelloWorldPlugin
    PRIVATE
        helloplugin.h
        helloplugin.cpp
)

target_link_libraries(HelloWorldPlugin PRIVATE Qt::QmlCompiler)
