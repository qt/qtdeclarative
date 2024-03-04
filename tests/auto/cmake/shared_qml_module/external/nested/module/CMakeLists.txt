
cmake_minimum_required(VERSION 3.16)

project(scheduler VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_add_qml_module(nested_module
    URI nested.module
    VERSION 1.0
    QML_FILES
        Test.qml
)
