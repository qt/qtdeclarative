!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += qml
SOURCES += $$PWD/qmldialcontrol.cpp
include($$PWD/deployment.pri)
