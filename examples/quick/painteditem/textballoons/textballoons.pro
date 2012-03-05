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

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons/TextBalloonPlugin
 
sources.files = textballoons.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/painteditem/textballoons/TextBalloonPlugin

INSTALLS = qdeclarativesources sources target
