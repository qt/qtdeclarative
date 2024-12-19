qt_add_library(confused_test_module STATIC)
qt_autogen_tools_initial_setup(confused_test_module)

qt_policy(SET QTP0001 NEW)
# Not QTP0004, since we have a manually written qmldir in a strange place

qt_add_qml_module(confused_test_module
    URI Confused
    VERSION 1.0
    QML_FILES
        Main.qml
        Test/test.js
    RESOURCES
        Test/qmldir
)

target_link_libraries(confused_test_module
    PRIVATE Qt6::Qml
)
