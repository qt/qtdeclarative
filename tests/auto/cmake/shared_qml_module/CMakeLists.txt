cmake_minimum_required(VERSION 3.16)

project(scheduler VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml)
qt_standard_project_setup(REQUIRES 6.8)

add_custom_target(custom_qmllint_target ALL)
set(QT_QMLLINT_ALL_TARGET custom_qmllint_target)

add_subdirectory(external)
add_subdirectory(Scheduler)
add_subdirectory(SchedulerApp)
add_subdirectory(tests)
