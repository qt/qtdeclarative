TEMPLATE = lib
CONFIG += plugin qmltypes
QT += qml quick

QML_IMPORT_NAME = TextBalloon
QML_IMPORT_MAJOR_VERSION = 1

TARGET = qmltextballoonplugin

HEADERS += \
    plugin.h \
    textballoon.h

SOURCES += textballoon.cpp

RESOURCES += textballoon.qrc

qmldir_output.files = qmldir
qmldir_output.path = $$OUT_PWD
COPIES += qmldir_output

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/$$QML_IMPORT_NAME
qmldir_install.files = qmldir
qmldir_install.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem/$$QML_IMPORT_NAME

INSTALLS += qmldir_install target

CONFIG += install_ok  # Do not cargo-cult this!
