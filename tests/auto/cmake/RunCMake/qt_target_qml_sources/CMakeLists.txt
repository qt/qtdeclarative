cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml Test)

include(${RunCMake_TEST}.cmake)
