TARGET  = qmlxmllistmodelplugin
TARGETPATH = QtQuick/XmlListModel
include(../qimportbase.pri)

QT+= declarative xmlpatterns declarative-private v8-private core-private

SOURCES += qdeclarativexmllistmodel.cpp plugin.cpp
HEADERS += qdeclarativexmllistmodel_p.h

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir