TEMPLATE = lib
CONFIG += qt plugin
QT += qml quick

TARGET = qmltextballoonplugin

HEADERS += TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

OTHER_FILES += textballoon.json

DESTDIR = TextBalloonPlugin

INSTALL_PATH = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/customitems/painteditem/
qdeclarativesources.files += \
    TextBalloonPlugin/qmldir TextBalloonPlugin/plugin.h
qdeclarativesources.path += $$INSTALL_PATH/TextBalloonPlugin
sources.files = textballoons.qml $$SOURCES textballoon.h $$OTHER_FILES painteditem.pro
sources.path = $$INSTALL_PATH
target.path = $$INSTALL_PATH/TextBalloonPlugin

INSTALLS = qdeclarativesources sources target
