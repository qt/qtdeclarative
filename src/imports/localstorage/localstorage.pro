TARGET  = qmllocalstorageplugin
TARGETPATH = QtQuick/LocalStorage
include(../qimportbase.pri)

QT += sql qml qml-private v8-private core-private

SOURCES += plugin.cpp

DESTDIR = $$QT.qml.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir