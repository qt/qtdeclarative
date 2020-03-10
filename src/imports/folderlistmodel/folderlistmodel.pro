CXX_MODULE = qml
TARGET  = qmlfolderlistmodelplugin
TARGETPATH = Qt/labs/folderlistmodel
QML_IMPORT_VERSION = $$QT_VERSION

QT = core-private qml-private

SOURCES += qquickfolderlistmodel.cpp plugin.cpp \
    fileinfothread.cpp
HEADERS += qquickfolderlistmodel.h \
    fileproperty_p.h \
    fileinfothread_p.h

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
