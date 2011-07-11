TEMPLATE = app
TARGET = qmlscene
DESTDIR= ../../bin

QT += declarative declarative-private qtquick1

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

SOURCES += main.cpp

CONFIG += console

symbian {
    TARGET.EPOCHEAPSIZE = 0x20000 0x5000000
}

DEFINES += QML_RUNTIME_TESTING
