qt_internal_add_manual_test(qquickdialog
    GUI
    SOURCES
        qquickdialog.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "CustomDialog.qml"
    "DialogLabel.qml"
    "Marker.qml"
    "qquickdialog.qml"
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(qquickdialog "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
