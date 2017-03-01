CXX_MODULE = qml
TARGET = sharedimageplugin
TARGETPATH = QtQuick/SharedImage
IMPORT_VERSION = 1.0

QT *= quick qml gui-private core-private

SOURCES += \
    plugin.cpp \
    sharedimageprovider.cpp \
    qsharedimageloader.cpp

HEADERS += \
    sharedimageprovider.h \
    qsharedimageloader_p.h

load(qml_plugin)
