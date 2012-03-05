TEMPLATE = lib
CONFIG += qt plugin
QT += qml quick

TARGET = qmltextballoonplugin

HEADERS += TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

DESTDIR = TextBalloonPlugin

qqmlsources.files += \
    TextBalloonPlugin/qmldir

qqmlsources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons/TextBalloonPlugin
 
sources.files = textballoons.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons/TextBalloonPlugin

INSTALLS = qqmlsources sources target
