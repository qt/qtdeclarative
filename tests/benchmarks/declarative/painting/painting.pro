requires(contains(QT_CONFIG,opengl))

QT += opengl
CONFIG += console
macx:CONFIG -= app_bundle

SOURCES += paintbenchmark.cpp
