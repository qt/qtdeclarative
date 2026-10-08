cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

add_subdirectory(Foo/Bar)
add_subdirectory(app)
