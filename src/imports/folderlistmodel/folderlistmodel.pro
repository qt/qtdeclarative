TARGET  = qmlfolderlistmodelplugin
TARGETPATH = Qt/labs/folderlistmodel
include(../qimportbase.pri)

QT += widgets qml

SOURCES += qquickfolderlistmodel.cpp plugin.cpp
HEADERS += qquickfolderlistmodel.h

DESTDIR = $$QT.qml.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
