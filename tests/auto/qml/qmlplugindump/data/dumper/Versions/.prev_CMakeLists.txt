# Generated from versions.pro.

#####################################################################
## Versions Generic Library:
#####################################################################

qt_add_cmake_library(Versions
    MODULE
    SOURCES
        versions.cpp versions.h
        versions_plugin.cpp versions_plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:versions.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:versions.pro:NOT _PRO_FILE_PWD____equals____ss_OUT_PWD:
# COPIES = "cpqmldir"
# cpqmldir.files = "qmldir" "plugins.qmltypes"
# cpqmldir.path = "$$OUT_PWD"

qt_autogen_tools_initial_setup(Versions)
