cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml)

qt_standard_project_setup(REQUIRES 6.11)

add_subdirectory(parent)
