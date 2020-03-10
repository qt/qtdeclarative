CXX_MODULE = qml
TARGET = sharedimageplugin
TARGETPATH = Qt/labs/sharedimage
QML_IMPORT_VERSION = $$QT_VERSION

QT *= quick-private qml gui-private core-private

SOURCES += \
    plugin.cpp \
    sharedimageprovider.cpp \
    qsharedimageloader.cpp

HEADERS += \
    sharedimageprovider.h \
    qsharedimageloader_p.h

CONFIG += qmltypes install_qmltypes
load(qml_plugin)
