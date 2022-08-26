# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from extendedtype.pro.

#####################################################################
## ExtendedType Generic Library:
#####################################################################

qt_internal_add_cmake_library(ExtendedType
    MODULE
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        plugin.cpp plugin.h
        types.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:extendedtype.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:extendedtype.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cp"
# cp.files = "qmldir" "plugins.qmltypes"
# cp.path = "$$OUT_PWD"

qt_autogen_tools_initial_setup(ExtendedType)
# special case begin
get_target_property(binary_dir ExtendedType BINARY_DIR)
file(COPY qmldir plugins.qmltypes
    DESTINATION ${binary_dir}
)
# special case end
