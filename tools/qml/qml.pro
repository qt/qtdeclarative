QT += qml gui core-private
qtHaveModule(widgets): QT += widgets

HEADERS += conf.h
SOURCES += main.cpp
RESOURCES += qml.qrc

mac {
    OTHER_FILES += Info.plist
    QMAKE_INFO_PLIST = Info.plist
    ICON = qml.icns
}

load(qt_tool)
