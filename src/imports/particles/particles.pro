TARGET  = qmlparticlesplugin
TARGETPATH = Qt/labs/particles
include(../qimportbase.pri)

HEADERS += \
    V1/qdeclarativeparticles_p.h 

SOURCES += \
    particles.cpp \
    V1/qdeclarativeparticles.cpp 

QT += declarative opengl core gui declarative-private core-private gui-private qtquick1 qtquick1-private widgets-private v8-private

OTHER_FILES += \
    qmldir

DESTDIR = $$QT.declarative.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
