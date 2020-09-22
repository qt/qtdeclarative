# Generated from plugin.pro.

#####################################################################
## testplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(testplugin
    MODULE
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:plugin.pro:<TRUE>:
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(testplugin)
