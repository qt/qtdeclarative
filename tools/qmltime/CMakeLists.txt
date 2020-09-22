# Generated from qmltime.pro.

#####################################################################
## qmltime Binary:
#####################################################################

qt_internal_add_executable(qmltime
    GUI
    SOURCES
        qmltime.cpp qmltime.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmltime.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Time"
# QML_IMPORT_NAME = "QmlTime"
# QML_IMPORT_VERSION = "1.0"
# TEMPLATE = "app"

## Scopes:
#####################################################################

set_target_properties(qmltime PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI QmlTime
)

qt6_qml_type_registration(qmltime)
