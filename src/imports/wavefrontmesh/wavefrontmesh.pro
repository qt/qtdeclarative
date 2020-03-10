CXX_MODULE = qml
TARGET  = qmlwavefrontmeshplugin
TARGETPATH = Qt/labs/wavefrontmesh
QML_IMPORT_VERSION = $$QT_VERSION

QT = core-private qml-private quick-private

SOURCES += \
    plugin.cpp \
    qwavefrontmesh.cpp

HEADERS += \
    qwavefrontmesh.h

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
