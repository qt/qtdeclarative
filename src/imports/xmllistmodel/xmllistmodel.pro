TARGET  = qmlxmllistmodelplugin
TARGETPATH = QtQuick/XmlListModel
include(../qimportbase.pri)

QT += network qml xmlpatterns qml-private v8-private core-private

SOURCES += qqmlxmllistmodel.cpp plugin.cpp
HEADERS += qqmlxmllistmodel_p.h

DESTDIR = $$QT.qml.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir