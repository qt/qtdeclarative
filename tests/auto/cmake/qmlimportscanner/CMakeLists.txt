
cmake_minimum_required(VERSION 3.1)
project(qis_test)

find_package(Qt5Qml 5.0.0 REQUIRED)
find_package(Qt5Gui 5.0.0 REQUIRED)
find_package(Qt5Test 5.0.0 REQUIRED)
find_package(Qt5QmlImportScanner REQUIRED)

set(CMAKE_CXXFLAGS "${CMAKE_CXXFLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(qis_test "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
                        "${CMAKE_CURRENT_SOURCE_DIR}/qis_test.qrc")
target_link_libraries(qis_test PRIVATE Qt5::Gui Qt5::Qml Qt5::Test)
qt5_import_qml_plugins(qis_test)
