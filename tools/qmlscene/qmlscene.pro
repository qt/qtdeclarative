TEMPLATE = app
TARGET = qmlscene
DESTDIR= $$QT.qml.bins

QT += qml quick core-private
!contains(QT_CONFIG, no-widgets):QT += widgets

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

SOURCES += main.cpp

CONFIG += console

DEFINES += QML_RUNTIME_TESTING QT_QML_DEBUG_NO_WARNING
