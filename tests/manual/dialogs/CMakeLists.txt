# Generated from dialogs.pro.

#####################################################################
## dialogs Binary:
#####################################################################

qt_internal_add_manual_test(dialogs
    GUI
    SOURCES
        dialogs.cpp
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
    "dialogs.qml"
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(dialogs "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:dialogs.pro:<TRUE>:
# TEMPLATE = "app"
