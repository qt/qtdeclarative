QT       = core
CONFIG  += console
CONFIG  -= app_bundle
DESTDIR  = $$QT.declarative.bins
SOURCES += main.cpp

include(../../src/declarative/qml/parser/parser.pri)


