cmake_minimum_required(VERSION 3.16...3.22)
project(qmlquery)

find_package(Qt6 REQUIRED COMPONENTS Qml)
set(CMAKE_AUTOMOC TRUE)

add_subdirectory(My/Things)
