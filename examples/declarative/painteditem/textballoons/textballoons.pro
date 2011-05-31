TEMPLATE = lib
CONFIG += qt plugin
QT += declarative

TARGET = qmltextballoonplugin

HEADERS += TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

DESTDIR = TextBalloonPlugin

qdeclarativesources.files += \
    TextBalloonPlugin/qmldir

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons/TextBalloonPlugin
 
sources.files = textballoons.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons/TextBalloonPlugin

INSTALLS = qdeclarativesources sources target
