TARGET = QtQuickParticles
MODULE = quickparticles

CONFIG += internal_module

QT = core-private gui-private qml-private quick-private

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
msvc:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS_PRIVATE += -lgcov
}

include(particles.pri)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick/Particles.2
QML_IMPORT_NAME = QtQuick.Particles
IMPORT_VERSION = 2.$$QT_MINOR_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes

load(qt_module)
