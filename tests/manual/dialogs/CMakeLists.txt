if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(dialogs LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(dialogs
    GUI
    SOURCES
        dialogs.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "dialogs.qml"
    "FileDialogPage.qml"
    "FontDialogPage.qml"
    "StringListView.qml"
    "qmldir"
    "Theme.qml"
)

qt_internal_add_resource(dialogs "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
