# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# See comment below why we do this conditionally.
set(main_qml "qml/main.qml")
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
    set(qml_extra_files "")
else()
    set(qml_extra_files "${main_qml}")
endif()

qt_add_library(Derived STATIC)
qt_add_qml_module(Derived
    URI "Derived"
    QML_FILES
        qml/Blue.qml
        ${qml_extra_files}
)

# The library has no cpp sources, prevent 'ranlib: has no symbols' warnings
set_target_properties(Derived PROPERTIES AUTOMOC OFF)

# Add one more qml source in a separate qt_target_qml_sources call,
# to ensure we correctly create an additional resource for the same
# qml module and that linking to the qml module plugin + calling
# Q_IMPORT_PLUGIN will initialize the resource.
# We can only do it with CMake 3.19+, otherwise we get a configure time
# error in qt6_target_qml_sources.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
    qt6_target_qml_sources(Derived
        QML_FILES
            ${main_qml}
    )
endif()

# Make sure we build the libraries before we link to them.
add_dependencies(Derived Base Baseplugin)

# Manually link to the library files, to avoid bringing in any
# object library dependencies.
target_link_libraries(Derived PRIVATE $<TARGET_FILE:Base>)
target_link_libraries(Derived PRIVATE $<TARGET_FILE:Baseplugin>)
