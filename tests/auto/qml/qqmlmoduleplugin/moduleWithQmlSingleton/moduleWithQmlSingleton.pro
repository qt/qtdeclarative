TEMPLATE = lib
CONFIG += plugin
SOURCES = plugin.cpp
QT = core qml
DESTDIR = ../imports/org/qtproject/ModuleWithQmlSingleton

QT += core-private gui-private qml-private

IMPORT_FILES = \
        qmldir \
        MySingleton.qml \
        MySingleton2.qml

include (../../../shared/imports.pri)

subfiles.files = internal/InternalType.qml
subfiles.path = $$DESTDIR/internal
COPIES += subfiles
