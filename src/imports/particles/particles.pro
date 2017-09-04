CXX_MODULE = qml
TARGET  = particlesplugin
TARGETPATH = QtQuick/Particles.2
IMPORT_VERSION = 2.0

greaterThan(QT_GCC_MAJOR_VERSION, 5):!qnx {
    # Our code is bad. Temporary workaround. Fixed in 5.8
    QMAKE_CXXFLAGS += -fno-delete-null-pointer-checks -fno-lifetime-dse
}

SOURCES += \
    plugin.cpp

QT += quick-private quickparticles-private qml-private

load(qml_plugin)
