qt_internal_add_test(tst_qmlbasicapp
    SOURCES
        tst_qmlbasicapp.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt6_add_qml_module(tst_qmlbasicapp
    VERSION 1.0
    URI "BasicApp"
    NO_CREATE_PLUGIN_TARGET
    QML_FILES
        main.qml
)

add_subdirectory(TimeExample)
