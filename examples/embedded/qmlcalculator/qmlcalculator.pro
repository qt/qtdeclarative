!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += qml
SOURCES += $$PWD/qmlcalculator.cpp
include($$PWD/deployment.pri)
