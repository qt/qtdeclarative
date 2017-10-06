QT = qml core-private
qtHaveModule(gui): QT += gui
qtHaveModule(widgets): QT += widgets

HEADERS += conf.h
SOURCES += main.cpp
RESOURCES += qml.qrc

QMAKE_TARGET_DESCRIPTION = QML Runtime

mac {
    OTHER_FILES += Info.plist
    QMAKE_INFO_PLIST = Info.plist
    ICON = qml.icns
}

qtConfig(qml-debug): DEFINES += QT_QML_DEBUG_NO_WARNING

load(qt_tool)
