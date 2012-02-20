TARGET  = qmlfolderlistmodelplugin
TARGETPATH = Qt/labs/folderlistmodel
include(../qimportbase.pri)

QT += declarative

SOURCES += qdeclarativefolderlistmodel.cpp plugin.cpp \
    fileinfothread.cpp
HEADERS += qdeclarativefolderlistmodel.h \
    fileproperty_p.h \
    fileinfothread_p.h

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
