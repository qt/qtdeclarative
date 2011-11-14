!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += declarative
SOURCES += $$PWD/qmleasing.cpp
include($$PWD/deployment.pri)
