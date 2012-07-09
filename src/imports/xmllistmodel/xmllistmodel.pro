CXX_MODULE = qml
TARGET  = qmlxmllistmodelplugin
TARGETPATH = QtQuick/XmlListModel

QT += network qml xmlpatterns qml-private v8-private core-private

SOURCES += qqmlxmllistmodel.cpp plugin.cpp
HEADERS += qqmlxmllistmodel_p.h

load(qml_plugin)
