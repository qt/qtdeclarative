# Generated from extendedtype.pro.

#####################################################################
## ExtendedType Generic Library:
#####################################################################

qt_add_cmake_library(ExtendedType
    MODULE
    SOURCES
        plugin.cpp plugin.h
        types.h
    PUBLIC_LIBRARIES
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
