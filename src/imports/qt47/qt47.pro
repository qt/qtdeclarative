TARGET  = qt47plugin
TARGETPATH = Qt
include(../qimportbase.pri)

SOURCES += \
    plugin.cpp

QT += declarative qtquick1 opengl qtquick1-private


OTHER_FILES += \
    qmldir

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
