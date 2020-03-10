CXX_MODULE = qml
TARGET  = qmlsettingsplugin
TARGETPATH = Qt/labs/settings
QML_IMPORT_VERSION = $$QT_VERSION

QT = core qml

HEADERS += \
    qqmlsettings_p.h

SOURCES += \
    plugin.cpp \
    qqmlsettings.cpp

CONFIG += qmltypes install_qmltypes

load(qml_plugin)
