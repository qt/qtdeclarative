!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += declarative
SOURCES += $$PWD/qmlcalculator.cpp
include($$PWD/deployment.pri)
