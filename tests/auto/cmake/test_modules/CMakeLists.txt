
cmake_minimum_required(VERSION 2.8)

project(test_modules)

# We have to find the dependencies separately because they are installed,
# whereas the modules under test are not installed yet, and are found in the
# build directory. Normally the Qt5 CMake files expect all modules in one
# installation prefix, so we work around that by finding the dependencies
# explicitly first.
find_package(Qt5Network REQUIRED)
find_package(Qt5Gui REQUIRED)

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

include_directories(
  ${Qt5Qml_INCLUDE_DIRS}
  ${Qt5Quick_INCLUDE_DIRS}
)

add_definitions(
  ${Qt5Qml_DEFINITIONS}
  ${Qt5Quick_DEFINITIONS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mainapp main.cpp)
target_link_libraries(mainapp
  ${Qt5Qml_LIBRARIES}
  ${Qt5Quick_LIBRARIES}
)
