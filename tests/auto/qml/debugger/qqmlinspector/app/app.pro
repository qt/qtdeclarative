TARGET = app
DESTDIR = $$OUT_PWD
QT += qml quick

CONFIG += qml_debug
macx:CONFIG -= app_bundle

SOURCES += main.cpp

OTHER_FILES += qtquick1.qml qtquick2.qml
