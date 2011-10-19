TARGET  = qmlfolderlistmodelplugin
TARGETPATH = Qt/labs/folderlistmodel
include(../qimportbase.pri)

QT += widgets declarative

SOURCES += qdeclarativefolderlistmodel.cpp plugin.cpp
HEADERS += qdeclarativefolderlistmodel.h

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
