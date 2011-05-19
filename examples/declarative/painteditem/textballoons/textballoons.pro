TEMPLATE = lib
CONFIG += qt plugin
QT += declarative

TARGET = qmltextballoonplugin

HEADERS += textballoonplugin/plugin.h \
    textballoonplugin/textballoon.h

SOURCES += textballoonplugin/textballoon.cpp

DESTDIR = textballoonplugin

qdeclarativesources.files += \
    textballoonplugin/qmldir

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons/textballoonplugin
 
sources.files = textballoons.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/painteditem/textballoons/textballoonplugin

INSTALLS = qdeclarativesources sources target
