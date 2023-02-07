# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(application_name tst_qmltc_examples)
#[[
#! [qmltc-app-name]
# Use "my_qmltc_example" as an application name:
set(application_name my_qmltc_example)

# Create a CMake target, add C++ source files, link libraries, etc...
#! [qmltc-app-name]
]]

qt_internal_add_test(${application_name}
    SOURCES
        tst_qmltc_examples.cpp
        colorpicker.h colorpicker.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Quick
        Qt::QuickTemplates2Private # special
        Qt::Gui
)

#! [qmltc-qml-files]
# Specify a list of QML files to be compiled:
set(application_qml_files
    myApp.qml
    MyButton.qml
    MySlider.qml
)
#! [qmltc-qml-files]

# files "invisible" to the documentation:
list(APPEND application_qml_files
    special/HelloWorld.qml
)
target_compile_definitions(${application_name} PRIVATE
    QT_USE_QSTRINGBUILDER
    QMLTC_TESTS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    QMLTC_TESTS_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

qt_policy(SET QTP0001 NEW)

#! [qmltc-add-qml-module]
# Make the application into a proper QML module:
qt6_add_qml_module(${application_name}
    URI QmltcExample
    QML_FILES ${application_qml_files}

    # Compile qml files (listed in QML_FILES) to C++ using qmltc and add these
    # files to the application binary:
    ENABLE_TYPE_COMPILER
)
#! [qmltc-add-qml-module]

#! [qmltc-compile-to-cpp]
# (qmltc-specific) Link *private* libraries that correspond to QML modules:
target_link_libraries(${application_name} PRIVATE Qt::QmlPrivate Qt::QuickPrivate)
#! [qmltc-compile-to-cpp]
