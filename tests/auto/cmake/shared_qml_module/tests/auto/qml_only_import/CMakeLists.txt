cmake_minimum_required(VERSION 3.16)

project(tst_qml_only_import VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Test)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(tst_qml_only_import
    tst_qml_only_import.cpp
)

# Main.qml imports nested.module purely in QML. We deliberately do NOT declare it
# via DEPENDENCIES/IMPORTS TARGET, so it has no entry in the CMake dependency
# graph - this exercises the scanner half of the union. IMPORT_PATH lets
# qmlimportscanner resolve the import; the add_dependencies() below makes sure the
# module is built before we scan.
qt_add_qml_module(tst_qml_only_import
    URI QmlOnlyImportApp
    VERSION 1.0
    IMPORT_PATH "${CMAKE_BINARY_DIR}/external"
    QML_FILES
        Main.qml
)

add_dependencies(tst_qml_only_import nested_module)

set_target_properties(tst_qml_only_import PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(tst_qml_only_import
    PRIVATE
        Qt::Quick
        Qt::Test
)

# The Qt-prefixed directories the test expects QLibraryInfo::path() to report for
# Qt's own binaries and helper tools. Passed in from CMake because the test has no
# other way to tell a Qt-prefixed path from an app-prefixed one.
target_compile_definitions(tst_qml_only_import PRIVATE
    EXPECTED_QT_BINARIES_PATH="${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
    EXPECTED_QT_LIBRARY_EXECUTABLES_PATH="${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}"
)
