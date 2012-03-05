!wince*:warning("DEPLOYMENT support required. This project only works on WinCE.")

QT += qml network
SOURCES += $$PWD/qmltwitter.cpp
include($$PWD/deployment.pri)
