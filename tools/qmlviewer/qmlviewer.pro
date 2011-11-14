TEMPLATE = app
CONFIG += qt uic declarative_debug
DESTDIR = $$QT.declarative.bins
QT += declarative qtquick1 qtquick1-private widgets widgets-private

include(qml.pri)

SOURCES += main.cpp

INCLUDEPATH += ../../include/QtDeclarative
INCLUDEPATH += ../../src/declarative/util
INCLUDEPATH += ../../src/declarative/graphicsitems
INCLUDEPATH += ../../src/3rdparty/v8/include

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    TARGET=QMLViewer
    ICON=qml.icns
} else {
    TARGET=qmlviewer
}
