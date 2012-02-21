TARGET  = qmllocalstorageplugin
TARGETPATH = QtQuick/LocalStorage
include(../qimportbase.pri)

QT += sql declarative declarative-private v8-private core-private

SOURCES += plugin.cpp

OTHER_FILES += localstorage.json

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
