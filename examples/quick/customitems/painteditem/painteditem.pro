TEMPLATE = lib
CONFIG += plugin qmltypes
QT += qml quick

QML_IMPORT_NAME = TextBalloonPlugin
QML_IMPORT_MAJOR_VERSION = 1

TARGET = qmltextballoonplugin

HEADERS += \
    TextBalloonPlugin/plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

RESOURCES += painteditem.qrc

DESTDIR = $$QML_IMPORT_NAME

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/$$QML_IMPORT_NAME
qmldir.files = $$QML_IMPORT_NAME/qmldir
qmldir.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/$$QML_IMPORT_NAME

INSTALLS += qmldir target

CONFIG += install_ok  # Do not cargo-cult this!

OTHER_FILES += \
    textballoons.qml
