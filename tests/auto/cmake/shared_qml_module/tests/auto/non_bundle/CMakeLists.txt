cmake_minimum_required(VERSION 3.16)

project(tst_non_bundle VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Test)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(tst_non_bundle
    tst_non_bundle.cpp
)

# Deliberately not a bundle (MACOSX_BUNDLE is left off): the generated qt.conf
# must land next to the executable rather than in a bundle Resources directory.
qt_add_qml_module(tst_non_bundle
    URI NonBundleApp
    DEPENDENCIES
        TARGET scheduler
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(tst_non_bundle
    PRIVATE
        Qt::Quick
        Qt::Test
)

# The Qt-prefixed directories the test expects QLibraryInfo::path() to report for
# Qt's own binaries and helper tools. Passed in from CMake because the test has no
# other way to tell a Qt-prefixed path from an app-prefixed one.
target_compile_definitions(tst_non_bundle PRIVATE
    EXPECTED_QT_BINARIES_PATH="${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
    EXPECTED_QT_LIBRARY_EXECUTABLES_PATH="${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}"
)
