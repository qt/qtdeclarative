TEMPLATE = lib
CONFIG += qt plugin
QT += qml quick

TARGET = qmltextballoonplugin

HEADERS += TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

DESTDIR = TextBalloonPlugin

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/TextBalloonPlugin
qmldir.files = TextBalloonPlugin/qmldir
qmldir.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/TextBalloonPlugin
qml.files = textballoons.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/

INSTALLS += qml qmldir target
