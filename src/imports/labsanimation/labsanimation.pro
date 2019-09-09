CXX_MODULE = qml
TARGET  = labsanimationplugin
TARGETPATH = Qt/labs/animation
IMPORT_VERSION = 1.0

SOURCES += \
    qquickboundaryrule.cpp \
    plugin.cpp

HEADERS += \
    qquickboundaryrule_p.h

QT = qml-private quick-private

load(qml_plugin)
