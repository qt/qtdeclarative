qt_add_qml_module(quick_automatically_added
    URI test
    SOURCES
        test.h test.cpp
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        Test.qml
)
target_link_libraries(quick_automatically_added PRIVATE Qt6::Quick)
