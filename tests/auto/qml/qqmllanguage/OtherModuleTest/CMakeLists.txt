qt_add_library(other_module STATIC)
qt_autogen_tools_initial_setup(other_module)

qt_policy(set QTP0001 NEW)

qt6_add_qml_module(other_module
    URI OtherModuleTest
    VERSION 1.0
    SOURCES
        other.h
)

qt_autogen_tools_initial_setup(other_moduleplugin)

add_dependencies(tst_qqmllanguage other_module other_moduleplugin)
