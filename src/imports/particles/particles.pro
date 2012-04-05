TARGET  = particlesplugin
TARGETPATH = QtQuick/Particles.2
include(../qimportbase.pri)

SOURCES += \
    plugin.cpp

QT += quick-private qml-private

OTHER_FILES += \
    qmldir

DESTDIR = $$QT.qml.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
