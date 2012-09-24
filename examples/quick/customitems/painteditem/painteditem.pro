TEMPLATE = lib
CONFIG += qt plugin
QT += qml quick

TARGET = qmltextballoonplugin

HEADERS += TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

OTHER_FILES += textballoon.json

DESTDIR = TextBalloonPlugin

qdeclarativesources.files += \
    TextBalloonPlugin/qmldir

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtquick/qml/painteditem/TextBalloonPlugin
 
sources.files = textballoons.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtquick/qml/painteditem
target.path += $$[QT_INSTALL_EXAMPLES]/qtquick/qml/painteditem/TextBalloonPlugin

INSTALLS = qdeclarativesources sources target
