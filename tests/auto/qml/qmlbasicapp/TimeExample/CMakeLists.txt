qt6_add_library(qmlqtimeexample STATIC)
qt_autogen_tools_initial_setup(qmlqtimeexample)
qt6_add_qml_module(qmlqtimeexample
    VERSION 1.0
    URI "TimeExample"
    SOURCES
        timemodel.cpp timemodel.h
    QML_FILES
        Clock.qml
    RESOURCES
        center.png
        clock.png
        hour.png
        minute.png
    DEPENDENCIES
        QtQml
)
qt_autogen_tools_initial_setup(qmlqtimeexampleplugin)
