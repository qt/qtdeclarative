qt_policy(SET QTP0001 NEW)
qt_add_library(tst_qqmllanguage_qmlmodule STATIC)
qt_autogen_tools_initial_setup(tst_qqmllanguage_qmlmodule)
qt_add_qml_module(tst_qqmllanguage_qmlmodule
    URI testhelper
    VERSION 1.0
    SOURCES
        "declarativelyregistered.h"
        "declarativelyregistered.cpp"
)

qt_autogen_tools_initial_setup(tst_qqmllanguage_qmlmoduleplugin)
target_link_libraries(tst_qqmllanguage PRIVATE tst_qqmllanguage_qmlmoduleplugin)
