QT = qml-private core-private
qtHaveModule(gui): QT += gui
qtHaveModule(widgets): QT += widgets

HEADERS += conf.h
SOURCES += main.cpp
RESOURCES += qml.qrc
CONFIG += qmltypes

QML_IMPORT_NAME = QmlRuntime.Config
QML_IMPORT_VERSION = 1.0

QMAKE_TARGET_DESCRIPTION = QML Runtime

ICON = resources/qml-64.png
win32 {
    RC_ICONS = resources/qml.ico
}
mac {
    OTHER_FILES += resources/Info.plist
    QMAKE_INFO_PLIST = resources/Info.plist
    ICON = resources/qml.icns
}

qtConfig(qml-debug): DEFINES += QT_QML_DEBUG_NO_WARNING

load(qt_tool)
