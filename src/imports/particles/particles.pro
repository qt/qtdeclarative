CXX_MODULE = qml
TARGET  = particlesplugin
TARGETPATH = QtQuick/Particles
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

QT += quick-private quickparticles-private qml-private

load(qml_plugin)
