cmake_minimum_required(VERSION 3.16)

project(tst_models LANGUAGES CXX)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Quick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(tst_models tst_models.cpp)

target_link_libraries(tst_models
    PRIVATE
        scheduler
        Qt6::Gui
        Qt6::Test
        Qt6::Quick
)


qt_add_qml_module(tst_models
    URI unittest
    DEPENDENCIES
        TARGET scheduler
)


qt_add_executable(tst_models_dummy_helper dummy.cpp)
qt_add_qml_module(tst_models_dummy_helper
    URI unimportant
    DEPENDENCIES
        TARGET nested_module
)
