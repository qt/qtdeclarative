cmake_minimum_required(VERSION 3.16)
enable_testing()

project(scheduler VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml)
qt_standard_project_setup(REQUIRES 6.8)

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    # _qt_internal_add_phony_target_deferred target explodes
    add_custom_target(custom_qmllint_target ALL)
    set(QT_QMLLINT_ALL_TARGET custom_qmllint_target)
endif()

add_subdirectory(external)
add_subdirectory(Scheduler)
add_subdirectory(SchedulerApp)
add_subdirectory(tests)

if(NOT WIN32)
    # We can't run the test on Windows without deploying it
    # or without otherwise ensuring that the Qt DLLs are found
    add_test(NAME tst_models COMMAND tst_models)
endif()
