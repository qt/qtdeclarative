
cmake_minimum_required(VERSION 2.8)
project(qqc_test)

find_package(Qt6Qml 6.0.0 REQUIRED)
find_package(Qt6Gui 6.0.0 REQUIRED)
find_package(Qt6Test 6.0.0 REQUIRED)
find_package(Qt6QuickCompiler)

set(CMAKE_CXXFLAGS "${CMAKE_CXXFLAGS} ${Qt6Core_EXECUTABLE_COMPILE_FLAGS}")

qtquick_compiler_add_resources(RESOURCES "empty.qrc" "resources with space/qqc_test.qrc")

set(CMAKE_AUTOMOC ON)

add_executable(qqc_test "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" ${RESOURCES})
target_link_libraries(qqc_test Qt6::Gui Qt6::Qml Qt6::Test)
