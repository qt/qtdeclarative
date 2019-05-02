QT -= gui
QT += qml
CONFIG += console
CONFIG -= app_bundle
SOURCES += main.cpp
LIBS += -fsanitize=fuzzer
