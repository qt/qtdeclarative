# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_link_qml_module_without_target)

find_package(Qt6 COMPONENTS REQUIRED
    Core
    Test
    Quick
    QuickLayouts
    QuickTestUtilsPrivate
)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(Base)
add_subdirectory(Derived)

qt6_add_executable(test_link_qml_module_without_target main.cpp)

target_link_libraries(test_link_qml_module_without_target PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::QuickPrivate
    Qt6::QuickTestUtilsPrivate
)

# Suppress running qmlimportscanner and auto linking of static qml plugins, to ensure we don't
# automatically link to the project-created qml module object library initializers.
# Doing that would be at odds with what the test is trying to test.
set_target_properties(test_link_qml_module_without_target PROPERTIES _QT_QML_PLUGINS_IMPORTED TRUE)

# Because we disable qmlimportscanner auto-linking above, we need to manually link to the Qt
# provided qml plugins used by the project. Note that we link to the plugins, and not the
# backing libraries, to ensure we link to the initializer object libraries as well
# (which are dependencies of the plugin targets).
if(NOT QT6_IS_SHARED_LIBS_BUILD)
    target_link_libraries(test_link_qml_module_without_target PRIVATE
        Qt6::qtquick2plugin
        Qt6::qquicklayoutsplugin
    )
endif()

# Make sure we build the libraries before we link to them.
add_dependencies(test_link_qml_module_without_target Base Baseplugin Derived Derivedplugin)

# Link against the library files and not the targets, so that we can confirm
# the ability to manually initialize the qml modules and their resources from inside main.cpp
# using Q_IMPORT_QML_PLUGIN calls.
# The order of libraries is important, the plugins need to come before the backing libraries
# so that linkers like 'Linux ld' don't discard symbols that haven't yet been marked as required.
target_link_libraries(test_link_qml_module_without_target PRIVATE
    $<TARGET_FILE:Baseplugin>
    $<TARGET_FILE:Base>
    $<TARGET_FILE:Derivedplugin>
    $<TARGET_FILE:Derived>
)
